#include "mainwindow.h"

#include "iapcodec.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QString formatBytes(qint64 size)
{
    if (size < 1024)
        return QString("%1 B").arg(size);
    return QString("%1 KB  (%2 B)").arg(size / 1024.0, 0, 'f', 1).arg(size);
}

QString hex32(quint32 value)
{
    return QString("0x%1").arg(QString("%1").arg(value, 8, 16, QChar('0')).toUpper());
}

QString vectorSummary(const QByteArray &image, bool *looksLikeApp)
{
    if (image.size() < 8) {
        if (looksLikeApp)
            *looksLikeApp = false;
        return QStringLiteral("文件不足 8 字节，没有向量表");
    }
    const quint32 sp = IapCodec::readU32(image, 0);
    const quint32 reset = IapCodec::readU32(image, 4);
    const bool spOk = (sp >= 0x20000000u && sp <= 0x20020000u)
            || (sp >= 0x10000000u && sp <= 0x10010000u);
    const quint32 resetAddr = reset & ~1u;
    const bool resetOk = (reset & 1u) && resetAddr >= 0x08010000u && resetAddr < 0x08080000u;
    if (looksLikeApp)
        *looksLikeApp = spOk && resetOk;
    const QString verdict = (spOk && resetOk)
            ? QStringLiteral("向量表在 App 区")
            : QStringLiteral("向量表不像链接在 0x08010000 的 App");
    return QString("SP %1    复位 %2    %3").arg(hex32(sp), hex32(reset), verdict);
}

QLabel *infoValue(const QString &text)
{
    auto *label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

const char *kStyle = R"(
QMainWindow, QWidget#central {
    background: #F3F5F7;
    color: #1F2328;
    font-family: "Microsoft YaHei UI", "Segoe UI";
    font-size: 13px;
}
QGroupBox {
    font-weight: 600;
    border: 1px solid #D0D7DE;
    border-radius: 8px;
    margin-top: 14px;
    padding: 12px 10px 10px 10px;
    background: #FFFFFF;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 4px;
}
QLineEdit, QComboBox {
    padding: 5px 8px;
    border: 1px solid #D0D7DE;
    border-radius: 6px;
    background: #FFFFFF;
    min-height: 22px;
}
QPushButton {
    padding: 6px 14px;
    border-radius: 6px;
    border: 1px solid #D0D7DE;
    background: #F6F8FA;
}
QPushButton:hover { background: #EEF3F8; }
QPushButton:disabled { color: #8C959F; background: #F6F8FA; }
QPushButton#primary {
    background: #1F6FEB;
    color: white;
    border: 1px solid #1F6FEB;
    font-weight: 600;
    padding: 8px 18px;
}
QPushButton#primary:hover { background: #1A61D0; }
QPushButton#primary:disabled { background: #9BB8E8; color: white; border-color: #9BB8E8; }
QPushButton#danger {
    background: #CF222E;
    color: white;
    border: 1px solid #CF222E;
}
QPushButton#danger:hover { background: #B31D28; }
QPushButton#danger:disabled { background: #E7B4B8; color: white; border-color: #E7B4B8; }
QPushButton#openBtn[connected="true"] {
    background: #9A6700;
    color: white;
    border: 1px solid #9A6700;
}
QPushButton#openBtn[connected="false"] {
    background: #1A7F37;
    color: white;
    border: 1px solid #1A7F37;
}
QProgressBar {
    border: 1px solid #D0D7DE;
    border-radius: 6px;
    background: #F6F8FA;
    text-align: center;
    min-height: 18px;
}
QProgressBar::chunk {
    background: #1F6FEB;
    border-radius: 5px;
}
QTextEdit#logView {
    background: #0D1117;
    color: #C9D1D9;
    border: 1px solid #30363D;
    border-radius: 6px;
    font-family: Consolas, "Cascadia Mono", monospace;
    font-size: 12px;
}
QStatusBar { background: #FFFFFF; border-top: 1px solid #D0D7DE; }
QLabel#title { font-size: 20px; font-weight: 700; }
QLabel#subtitle { color: #656D76; }
QLabel#vectorOk { color: #1A7F37; }
QLabel#vectorBad { color: #9A6700; }
)";

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new IapClient(this))
{
    setAcceptDrops(true);
    buildUi();
    setStyleSheet(QString::fromUtf8(kStyle));
    refreshPorts();
    loadSettings();
    updateActions();

    connect(m_client, &IapClient::logMessage, this, &MainWindow::appendLog);
    connect(m_client, &IapClient::portOpenChanged, this, [this](bool) { updateActions(); });
    connect(m_client, &IapClient::busyChanged, this, [this](bool) { updateActions(); });
    connect(m_client, &IapClient::deviceInfoChanged, this, &MainWindow::showDeviceInfo);
    connect(m_client, &IapClient::progressChanged, this, &MainWindow::onProgress);
    connect(m_client, &IapClient::upgradeFinished, this, &MainWindow::onUpgradeFinished, Qt::QueuedConnection);
    connect(m_client, &IapClient::notice, this, &MainWindow::onNotice);

    appendLog(LogInfo, QStringLiteral("就绪。打开串口，选择链接在 0x08010000 的 bin，然后开始升级。"));
    appendLog(LogInfo, QStringLiteral("流程：GET_INFO → START（擦除）→ DATA → END → JUMP。超时会按同一序号重发。"));
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("STM32 串口在线升级"));
    resize(1080, 760);
    setMinimumSize(920, 640);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
    fileMenu->addAction(QStringLiteral("打开固件..."), this, &MainWindow::browseFirmware);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出"), this, &QWidget::close);
    auto *helpMenu = menuBar()->addMenu(QStringLiteral("帮助"));
    helpMenu->addAction(QStringLiteral("升级说明"), this, &MainWindow::showHelp);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("STM32 串口在线升级"));
    title->setObjectName(QStringLiteral("title"));
    auto *subtitle = new QLabel(QStringLiteral("UART IAP    STM32F407VETx    App 0x08010000    115200 8N1    一问一答"));
    subtitle->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(title);
    root->addWidget(subtitle);

    auto *portGroup = new QGroupBox(QStringLiteral("串口"));
    auto *portLayout = new QHBoxLayout(portGroup);
    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(280);
    m_baudCombo = new QComboBox;
    const int bauds[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    for (int baud : bauds)
        m_baudCombo->addItem(QString::number(baud), baud);
    m_baudCombo->setCurrentIndex(m_baudCombo->findData(115200));
    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    m_openBtn = new QPushButton(QStringLiteral("打开串口"));
    m_openBtn->setObjectName(QStringLiteral("openBtn"));
    portLayout->addWidget(new QLabel(QStringLiteral("端口")));
    portLayout->addWidget(m_portCombo, 1);
    portLayout->addWidget(m_refreshBtn);
    portLayout->addSpacing(8);
    portLayout->addWidget(new QLabel(QStringLiteral("波特率")));
    portLayout->addWidget(m_baudCombo);
    portLayout->addWidget(m_openBtn);
    root->addWidget(portGroup);

    auto *columns = new QHBoxLayout;
    root->addLayout(columns);

    auto *fileGroup = new QGroupBox(QStringLiteral("固件"));
    auto *fileLayout = new QVBoxLayout(fileGroup);
    auto *fileRow = new QHBoxLayout;
    m_fileEdit = new QLineEdit;
    m_fileEdit->setPlaceholderText(QStringLiteral("选择或拖入 .bin 文件"));
    m_browseBtn = new QPushButton(QStringLiteral("浏览..."));
    fileRow->addWidget(m_fileEdit, 1);
    fileRow->addWidget(m_browseBtn);
    m_fileMetaLabel = new QLabel(QStringLiteral("未选择固件"));
    m_fileMetaLabel->setWordWrap(true);
    m_vectorLabel = new QLabel(QStringLiteral("向量表：—"));
    m_vectorLabel->setWordWrap(true);
    m_vectorLabel->setObjectName(QStringLiteral("vectorBad"));
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setFormat(QStringLiteral("%p%"));
    m_progressLabel = new QLabel(QStringLiteral("等待升级"));
    m_progressLabel->setWordWrap(true);

    auto *actionRow = new QHBoxLayout;
    m_pingBtn = new QPushButton(QStringLiteral("链路测试"));
    m_infoBtn = new QPushButton(QStringLiteral("读取设备"));
    m_jumpBtn = new QPushButton(QStringLiteral("跳转 App"));
    m_abortBtn = new QPushButton(QStringLiteral("中止"));
    m_abortBtn->setObjectName(QStringLiteral("danger"));
    actionRow->addWidget(m_pingBtn);
    actionRow->addWidget(m_infoBtn);
    actionRow->addWidget(m_jumpBtn);
    actionRow->addStretch();
    actionRow->addWidget(m_abortBtn);

    m_upgradeBtn = new QPushButton(QStringLiteral("开始升级"));
    m_upgradeBtn->setObjectName(QStringLiteral("primary"));
    m_autoJump = new QCheckBox(QStringLiteral("校验通过后自动 JUMP"));
    m_autoJump->setChecked(true);

    fileLayout->addLayout(fileRow);
    fileLayout->addWidget(m_fileMetaLabel);
    fileLayout->addWidget(m_vectorLabel);
    fileLayout->addWidget(m_progress);
    fileLayout->addWidget(m_progressLabel);
    fileLayout->addLayout(actionRow);
    fileLayout->addWidget(m_upgradeBtn);
    fileLayout->addWidget(m_autoJump);
    columns->addWidget(fileGroup, 3);

    auto *infoGroup = new QGroupBox(QStringLiteral("设备信息"));
    auto *form = new QFormLayout(infoGroup);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);
    m_protoLabel = infoValue(QStringLiteral("—"));
    m_bootLabel = infoValue(QStringLiteral("—"));
    m_baseLabel = infoValue(QStringLiteral("—"));
    m_maxLabel = infoValue(QStringLiteral("—"));
    m_payloadLabel = infoValue(QStringLiteral("—"));
    m_validLabel = infoValue(QStringLiteral("—"));
    m_appSizeLabel = infoValue(QStringLiteral("—"));
    m_appCrcLabel = infoValue(QStringLiteral("—"));
    form->addRow(QStringLiteral("协议版本"), m_protoLabel);
    form->addRow(QStringLiteral("Boot 版本"), m_bootLabel);
    form->addRow(QStringLiteral("App 起始"), m_baseLabel);
    form->addRow(QStringLiteral("App 容量"), m_maxLabel);
    form->addRow(QStringLiteral("单包最大"), m_payloadLabel);
    form->addRow(QStringLiteral("App 有效"), m_validLabel);
    form->addRow(QStringLiteral("App 大小"), m_appSizeLabel);
    form->addRow(QStringLiteral("App CRC32"), m_appCrcLabel);
    auto *hint = new QLabel(QStringLiteral("Boot 占 0x08000000 起 64KB，升级只擦写 App。镜像必须是原始 bin，向量表在文件开头。"));
    hint->setWordWrap(true);
    hint->setObjectName(QStringLiteral("subtitle"));
    form->addRow(hint);
    columns->addWidget(infoGroup, 2);

    auto *logGroup = new QGroupBox(QStringLiteral("通讯日志"));
    auto *logLayout = new QVBoxLayout(logGroup);
    auto *logBar = new QHBoxLayout;
    m_verbose = new QCheckBox(QStringLiteral("记录 DATA 原始报文"));
    auto *clearBtn = new QPushButton(QStringLiteral("清空日志"));
    logBar->addWidget(new QLabel(QStringLiteral("发送为蓝色，接收为绿色。控制命令始终显示十六进制。")));
    logBar->addStretch();
    logBar->addWidget(m_verbose);
    logBar->addWidget(clearBtn);
    m_log = new QTextEdit;
    m_log->setObjectName(QStringLiteral("logView"));
    m_log->setReadOnly(true);
    m_log->setUndoRedoEnabled(false);
    m_log->document()->setMaximumBlockCount(8000);
    logLayout->addLayout(logBar);
    logLayout->addWidget(m_log, 1);
    root->addWidget(logGroup, 1);

    m_statusLabel = new QLabel(QStringLiteral("串口未打开"));
    statusBar()->addWidget(m_statusLabel, 1);

    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::togglePort);
    connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::browseFirmware);
    connect(m_fileEdit, &QLineEdit::editingFinished, this, [this]() { loadImage(m_fileEdit->text().trimmed(), false); });
    connect(m_pingBtn, &QPushButton::clicked, m_client, &IapClient::ping);
    connect(m_infoBtn, &QPushButton::clicked, m_client, &IapClient::queryInfo);
    connect(m_jumpBtn, &QPushButton::clicked, m_client, &IapClient::jumpToApp);
    connect(m_abortBtn, &QPushButton::clicked, m_client, &IapClient::abortSession);
    connect(m_upgradeBtn, &QPushButton::clicked, this, &MainWindow::startUpgrade);
    connect(m_verbose, &QCheckBox::toggled, m_client, &IapClient::setVerboseHex);
    connect(clearBtn, &QPushButton::clicked, m_log, &QTextEdit::clear);
    connect(m_autoJump, &QCheckBox::toggled, this, [this](bool) { saveSettings(); });
    connect(m_verbose, &QCheckBox::toggled, this, [this](bool) { saveSettings(); });
}

void MainWindow::refreshPorts()
{
    const QString current = m_portCombo->currentData().toString();
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString label = info.portName();
        if (!info.description().isEmpty())
            label += QString("    %1").arg(info.description());
        m_portCombo->addItem(label, info.portName());
    }
    if (m_portCombo->count() == 0)
        m_portCombo->addItem(QStringLiteral("未发现串口"), QString());
    const int index = m_portCombo->findData(current);
    if (index >= 0)
        m_portCombo->setCurrentIndex(index);
    updateActions();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    const int baud = settings.value(QStringLiteral("baud"), 115200).toInt();
    const int baudIndex = m_baudCombo->findData(baud);
    if (baudIndex >= 0)
        m_baudCombo->setCurrentIndex(baudIndex);
    const QString port = settings.value(QStringLiteral("port")).toString();
    const int portIndex = m_portCombo->findData(port);
    if (portIndex >= 0)
        m_portCombo->setCurrentIndex(portIndex);
    m_autoJump->setChecked(settings.value(QStringLiteral("autoJump"), true).toBool());
    m_verbose->setChecked(settings.value(QStringLiteral("verbose"), false).toBool());
    m_client->setVerboseHex(m_verbose->isChecked());
    const QString file = settings.value(QStringLiteral("file")).toString();
    if (!file.isEmpty() && QFileInfo::exists(file)) {
        m_fileEdit->setText(file);
        loadImage(file, false);
    }
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("port"), m_portCombo->currentData().toString());
    settings.setValue(QStringLiteral("baud"), m_baudCombo->currentData().toInt());
    settings.setValue(QStringLiteral("file"), m_fileEdit->text().trimmed());
    settings.setValue(QStringLiteral("autoJump"), m_autoJump->isChecked());
    settings.setValue(QStringLiteral("verbose"), m_verbose->isChecked());
}

void MainWindow::updateActions()
{
    const bool open = m_client->isOpen();
    const bool busy = m_client->isBusy();
    const bool hasPort = !m_portCombo->currentData().toString().isEmpty();
    m_portCombo->setEnabled(!open);
    m_baudCombo->setEnabled(!open);
    m_refreshBtn->setEnabled(!open);
    m_openBtn->setEnabled(open || hasPort);
    m_openBtn->setText(open ? QStringLiteral("关闭串口") : QStringLiteral("打开串口"));
    m_openBtn->setProperty("connected", open);
    m_openBtn->style()->unpolish(m_openBtn);
    m_openBtn->style()->polish(m_openBtn);
    m_fileEdit->setEnabled(!busy);
    m_browseBtn->setEnabled(!busy);
    m_pingBtn->setEnabled(open && !busy);
    m_infoBtn->setEnabled(open && !busy);
    m_jumpBtn->setEnabled(open && !busy);
    m_upgradeBtn->setEnabled(open && !busy && !m_image.isEmpty());
    m_abortBtn->setEnabled(open);
    m_autoJump->setEnabled(!busy);
    if (open)
        m_statusLabel->setText(QString("已连接  %1  %2  8N1").arg(m_client->portName()).arg(m_baudCombo->currentData().toInt()));
    else if (!busy)
        m_statusLabel->setText(QStringLiteral("串口未打开"));
}

void MainWindow::loadImage(const QString &path, bool warn)
{
    if (path.isEmpty()) {
        m_image.clear();
        m_fileMetaLabel->setText(QStringLiteral("未选择固件"));
        m_vectorLabel->setText(QStringLiteral("向量表：—"));
        updateActions();
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_image.clear();
        m_fileMetaLabel->setText(QStringLiteral("无法读取文件"));
        if (warn)
            QMessageBox::warning(this, QStringLiteral("打开固件"), file.errorString());
        updateActions();
        return;
    }
    const QByteArray image = file.readAll();
    if (image.isEmpty()) {
        m_image.clear();
        m_fileMetaLabel->setText(QStringLiteral("文件是空的"));
        if (warn)
            QMessageBox::warning(this, QStringLiteral("打开固件"), QStringLiteral("文件是空的"));
        updateActions();
        return;
    }
    if (image.size() > 2 * 1024 * 1024) {
        m_image.clear();
        m_fileMetaLabel->setText(QStringLiteral("文件超过 2MB"));
        if (warn)
            QMessageBox::warning(this, QStringLiteral("打开固件"), QStringLiteral("文件超过 2MB"));
        updateActions();
        return;
    }

    m_image = image;
    m_fileEdit->setText(QFileInfo(path).absoluteFilePath());
    const quint32 crc = IapCodec::crc32IsoHdlc(m_image);
    m_fileMetaLabel->setText(QString("大小  %1    CRC32  %2").arg(formatBytes(m_image.size()), hex32(crc)));
    bool looksLikeApp = false;
    m_vectorLabel->setText(vectorSummary(m_image, &looksLikeApp));
    m_vectorLabel->setObjectName(looksLikeApp ? QStringLiteral("vectorOk") : QStringLiteral("vectorBad"));
    m_vectorLabel->style()->unpolish(m_vectorLabel);
    m_vectorLabel->style()->polish(m_vectorLabel);
    m_progress->setRange(0, int(qMin(m_image.size(), qint64(0x7FFFFFFF))));
    m_progress->setValue(0);
    m_progressLabel->setText(QStringLiteral("固件已载入，等待升级"));
    appendLog(LogInfo, QString("已载入 %1  %2  CRC32 %3")
                               .arg(QFileInfo(path).fileName(), formatBytes(m_image.size()), hex32(crc)));
    saveSettings();
    updateActions();
}

void MainWindow::appendLog(int level, const QString &text)
{
    QString color = QStringLiteral("#C9D1D9");
    switch (level) {
    case LogTx:
        color = QStringLiteral("#79C0FF");
        break;
    case LogRx:
        color = QStringLiteral("#7EE787");
        break;
    case LogOk:
        color = QStringLiteral("#56D364");
        break;
    case LogWarn:
        color = QStringLiteral("#E3B341");
        break;
    case LogError:
        color = QStringLiteral("#FF7B72");
        break;
    default:
        break;
    }
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const QString html = QString("<span style=\"color:#8B949E\">%1</span>  <span style=\"color:%2\">%3</span>")
                                 .arg(stamp, color, text.toHtmlEscaped());
    m_log->append(html);
}

void MainWindow::showDeviceInfo(const DeviceInfo &info)
{
    m_protoLabel->setText(QString("v%1").arg(info.protoVer));
    m_bootLabel->setText(QString("0x%1").arg(QString("%1").arg(info.bootVer, 4, 16, QChar('0')).toUpper()));
    m_baseLabel->setText(hex32(info.appBase));
    m_maxLabel->setText(formatBytes(info.appMax));
    m_payloadLabel->setText(QString("%1 字节").arg(info.maxPayload));
    m_validLabel->setText(info.appValid ? QStringLiteral("有效") : QStringLiteral("无效"));
    m_appSizeLabel->setText(formatBytes(info.appSize));
    m_appCrcLabel->setText(hex32(info.appCrc32));
}

void MainWindow::onProgress(qint64 sent, qint64 total, const QString &text, bool indeterminate)
{
    if (indeterminate) {
        m_progress->setRange(0, 0);
    } else {
        const int maximum = int(qMax(qint64(1), qMin(total, qint64(0x7FFFFFFF))));
        m_progress->setRange(0, maximum);
        m_progress->setValue(int(qBound(qint64(0), sent, qint64(maximum))));
    }
    m_progressLabel->setText(text);
    if (m_client->isBusy())
        m_statusLabel->setText(text);
}

void MainWindow::onUpgradeFinished(bool ok, const QString &message)
{
    m_statusLabel->setText(message);
    updateActions();
    if (message == QStringLiteral("已中止"))
        return;
    if (ok)
        QMessageBox::information(this, QStringLiteral("升级完成"), message);
    else
        QMessageBox::warning(this, QStringLiteral("升级未完成"), message);
}

void MainWindow::onNotice(bool ok, const QString &message)
{
    Q_UNUSED(ok);
    m_statusLabel->setText(message);
}

void MainWindow::togglePort()
{
    if (m_client->isOpen()) {
        m_client->closePort();
        appendLog(LogInfo, QStringLiteral("串口已关闭"));
        saveSettings();
        updateActions();
        return;
    }
    const QString name = m_portCombo->currentData().toString();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("打开串口"), QStringLiteral("没有可用串口"));
        return;
    }
    const QString error = m_client->openPort(name, m_baudCombo->currentData().toInt());
    if (!error.isEmpty())
        QMessageBox::warning(this, QStringLiteral("打开串口"), error);
    else
        saveSettings();
    updateActions();
}

void MainWindow::browseFirmware()
{
    const QString path = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("选择固件"),
            m_fileEdit->text(),
            QStringLiteral("固件 (*.bin);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;
    m_fileEdit->setText(path);
    loadImage(path, true);
}

void MainWindow::startUpgrade()
{
    loadImage(m_fileEdit->text().trimmed(), true);
    if (m_image.isEmpty())
        return;
    bool looksLikeApp = false;
    const QString summary = vectorSummary(m_image, &looksLikeApp);
    if (!looksLikeApp) {
        const auto answer = QMessageBox::question(
                this,
                QStringLiteral("固件检查"),
                summary + QStringLiteral("\n设备会在 END 时检查向量表。继续升级？"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    m_client->startUpgrade(m_image, m_autoJump->isChecked());
}

void MainWindow::showHelp()
{
    QMessageBox::information(
            this,
            QStringLiteral("升级说明"),
            QStringLiteral(
                    "1. 用 115200 8N1 打开串口。\n"
                    "2. 选择链接地址为 0x08010000 的原始 bin。\n"
                    "3. 点击开始升级。\n\n"
                    "主机按报文序号一问一答：GET_INFO、START、DATA、END、JUMP。\n"
                    "START 擦除最长等待 20 秒，其余命令等待 500 毫秒。超时后用同一序号重发。\n"
                    "单包数据最大 256 字节，除最后一包外长度是 4 的倍数。\n"
                    "中止会发送 ABORT，已经擦写的 App 会被设备视为无效。"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_client->isOpen())
        m_client->closePort();
    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty() || m_client->isBusy())
        return;
    const QString path = urls.first().toLocalFile();
    if (path.isEmpty())
        return;
    m_fileEdit->setText(path);
    loadImage(path, true);
}
