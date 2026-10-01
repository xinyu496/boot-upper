#include "iapclient.h"

IapClient::IapClient(QObject *parent)
    : QObject(parent)
    , m_port(new QSerialPort(this))
    , m_timer(this)
{
    qRegisterMetaType<DeviceInfo>();
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &IapClient::onTimeout);
    connect(m_port, &QSerialPort::readyRead, this, &IapClient::onReadyRead);
    connect(m_port, &QSerialPort::errorOccurred, this, &IapClient::onPortError);
}

bool IapClient::isOpen() const
{
    return m_port->isOpen();
}

bool IapClient::isBusy() const
{
    return m_busy;
}

QString IapClient::portName() const
{
    return m_port->portName();
}

void IapClient::setVerboseHex(bool enabled)
{
    m_verbose = enabled;
}

QString IapClient::openPort(const QString &name, int baud)
{
    if (m_busy)
        return QStringLiteral("当前有命令在执行");
    if (m_port->isOpen())
        closePort();

    m_port->setPortName(name);
    m_port->setBaudRate(baud);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port->open(QIODevice::ReadWrite)) {
        const QString reason = m_port->errorString();
        log(LogError, QString("打开 %1 失败：%2").arg(name, reason));
        return reason;
    }

    m_port->clear(QSerialPort::AllDirections);
    m_rx.clear();
    m_seq = 0;
    m_pending = Pending::None;
    log(LogOk, QString("串口已打开  %1  %2  8N1，报文序号从 1 开始").arg(name).arg(baud));
    emit portOpenChanged(true);
    return {};
}

void IapClient::closePort()
{
    m_timer.stop();
    ++m_epoch;
    const bool wasUpgrading = m_upgrading;
    m_pending = Pending::None;
    m_upgrading = false;
    m_deviceSession = false;
    m_needAbortOnFail = false;
    m_inFailAbort = false;
    m_postUpgradeInfo = false;
    m_userCancel = false;
    if (m_port->isOpen())
        m_port->close();
    m_rx.clear();
    setBusy(false);
    emit portOpenChanged(false);
    if (wasUpgrading) {
        log(LogError, QStringLiteral("串口已关闭，升级中断"));
        emit upgradeFinished(false, QStringLiteral("串口已关闭，升级中断"));
    }
}

void IapClient::ping()
{
    if (!isOpen() || m_busy)
        return;
    log(LogInfo, QStringLiteral("链路测试"));
    sendNew(IapCmd::Ping, {}, Pending::Ping);
}

void IapClient::queryInfo()
{
    if (!isOpen() || m_busy)
        return;
    log(LogInfo, QStringLiteral("读取设备信息"));
    sendNew(IapCmd::GetInfo, {}, Pending::GetInfo);
}

void IapClient::startUpgrade(const QByteArray &image, bool autoJump)
{
    if (!isOpen() || m_busy)
        return;
    if (image.isEmpty()) {
        log(LogError, QStringLiteral("固件为空"));
        emit notice(false, QStringLiteral("固件为空"));
        return;
    }
    if (image.size() > 2 * 1024 * 1024) {
        log(LogError, QStringLiteral("固件超过 2MB，已取消"));
        emit notice(false, QStringLiteral("固件过大"));
        return;
    }

    m_image = image;
    m_imageCrc = IapCodec::crc32IsoHdlc(m_image);
    m_autoJump = autoJump;
    m_offset = 0;
    m_sentOffset = 0;
    m_sentLength = 0;
    m_resyncs = 0;
    m_maxPayload = IapCodec::kMaxDataPayload;
    m_upgrading = true;
    m_deviceSession = false;
    m_needAbortOnFail = false;
    m_inFailAbort = false;
    m_userCancel = false;
    m_postUpgradeInfo = false;
    m_failureReason.clear();
    m_upgradeTimer.start();
    log(LogInfo, QString("开始升级  大小 %1 字节  CRC32 %2").arg(m_image.size()).arg(hex32(m_imageCrc)));
    emit progressChanged(0, m_image.size(), QStringLiteral("正在读取设备信息"), false);
    sendNew(IapCmd::GetInfo, {}, Pending::GetInfo);
}

void IapClient::abortSession()
{
    if (!isOpen() || m_pending == Pending::Abort)
        return;
    const bool cancelUpgrade = m_upgrading || m_deviceSession || m_needAbortOnFail;
    m_userCancel = cancelUpgrade;
    m_upgrading = false;
    m_postUpgradeInfo = false;
    m_failureReason.clear();
    m_inFailAbort = false;
    log(LogWarn, cancelUpgrade ? QStringLiteral("用户中止升级") : QStringLiteral("发送 ABORT"));
    sendNew(IapCmd::Abort, {}, Pending::Abort);
}

void IapClient::jumpToApp()
{
    if (!isOpen() || m_busy)
        return;
    log(LogInfo, QStringLiteral("请求跳转 App"));
    sendNew(IapCmd::Jump, {}, Pending::Jump);
}

void IapClient::sendNew(IapCmd cmd, const QByteArray &payload, Pending pending, bool resetBusyCount)
{
    m_timer.stop();
    ++m_epoch;
    m_retries = 0;
    if (resetBusyCount)
        m_busyRetries = 0;
    m_lastPayload = payload;
    m_waitSeq = (m_seq == 0 || m_seq == 0xFFFF) ? 1 : quint16(m_seq + 1);
    m_seq = m_waitSeq;
    m_waitCmd = quint8(cmd);
    m_pending = pending;
    m_lastFrame = IapCodec::encode(m_waitSeq, m_waitCmd, payload);
    if (m_lastFrame.isEmpty()) {
        failCurrent(QStringLiteral("报文组帧失败"));
        return;
    }
    setBusy(true);
    logTx();
    if (!writeFrame(false)) {
        failCurrent(QStringLiteral("串口发送失败"));
        return;
    }
    m_timer.start(timeoutFor(pending));
}

bool IapClient::writeFrame(bool retry)
{
    if (!m_port->isOpen())
        return false;
    if (retry)
        log(LogWarn, QString("重发 %1  序号=%2").arg(IapCodec::commandTitle(m_waitCmd)).arg(m_waitSeq));
    const qint64 written = m_port->write(m_lastFrame);
    return written == m_lastFrame.size();
}

void IapClient::onReadyRead()
{
    m_rx.append(m_port->readAll());
    while (true) {
        const IapTakeResult taken = IapCodec::takeFrame(m_rx);
        if (taken.status == IapTakeStatus::NeedMore)
            break;
        if (taken.status == IapTakeStatus::BadFrame) {
            log(LogWarn, QString("丢弃非法帧：%1").arg(taken.reason));
            continue;
        }
        handleFrame(taken.frame);
    }
}

void IapClient::handleFrame(const IapFrame &frame)
{
    QString line = QString("接收 %1  序号=%2  %3 字节")
                           .arg(IapCodec::commandTitle(frame.cmd))
                           .arg(frame.seq)
                           .arg(frame.raw.size());
    if (!frame.payload.isEmpty())
        line += QString("  状态=%1").arg(IapCodec::statusName(quint8(frame.payload.at(0))));
    if (frame.cmd == quint8(IapCmd::Data) && frame.payload.size() >= 5) {
        line += QString("  期望偏移=%1").arg(hex32(IapCodec::readU32(frame.payload, 1)));
    }
    const bool okStatus = !frame.payload.isEmpty() && quint8(frame.payload.at(0)) == 0x00;
    log(okStatus ? LogRx : LogWarn, line);
    if (m_verbose || frame.raw.size() <= 40)
        log(LogRx, QString("  %1").arg(IapCodec::toHex(frame.raw)));

    if (m_pending == Pending::None)
        return;
    if (frame.seq != m_waitSeq || frame.cmd != m_waitCmd) {
        log(LogWarn, QString("忽略不匹配的应答，当前等待 %1 序号=%2")
                             .arg(IapCodec::commandTitle(m_waitCmd))
                             .arg(m_waitSeq));
        return;
    }
    if (frame.payload.isEmpty()) {
        failCurrent(QStringLiteral("应答没有状态码"));
        return;
    }

    m_timer.stop();
    m_retries = 0;
    const quint8 status = quint8(frame.payload.at(0));
    if (status == 0x0A) {
        handleBusy();
        return;
    }

    const Pending which = m_pending;
    m_pending = Pending::None;
    handleReply(which, status, frame.payload);
}

void IapClient::handleReply(Pending which, quint8 status, const QByteArray &payload)
{
    if (status != 0x00 && !(which == Pending::Data && status == 0x05)) {
        QString reason = QString("%1 失败：%2").arg(IapCodec::commandTitle(m_waitCmd), IapCodec::statusName(status));
        if (status == 0x05 && payload.size() >= 5)
            reason += QString("，设备期望偏移 %1").arg(hex32(IapCodec::readU32(payload, 1)));
        if (which == Pending::Start)
            m_needAbortOnFail = false;
        if (which == Pending::End || which == Pending::Abort || which == Pending::Jump)
            m_deviceSession = false;
        if (m_postUpgradeInfo) {
            m_postUpgradeInfo = false;
            m_upgrading = false;
            log(LogWarn, QStringLiteral("升级已写入，刷新设备信息失败"));
            finishUpgrade(true, QString("升级完成，用时 %1 秒").arg(m_upgradeTimer.elapsed() / 1000.0, 0, 'f', 1));
            return;
        }
        if (m_upgrading || m_deviceSession || m_needAbortOnFail || m_inFailAbort || which == Pending::Abort) {
            if (which == Pending::Abort) {
                const bool userCancel = m_userCancel;
                const QString stored = m_failureReason;
                m_inFailAbort = false;
                m_userCancel = false;
                m_failureReason.clear();
                setBusy(false);
                if (!stored.isEmpty()) {
                    log(LogError, reason);
                    emit upgradeFinished(false, stored);
                } else if (userCancel) {
                    log(LogWarn, reason);
                    emit upgradeFinished(false, QStringLiteral("已中止"));
                } else {
                    log(LogWarn, reason);
                    emit notice(false, reason);
                }
                return;
            }
            failUpgrade(reason);
            return;
        }
        setBusy(false);
        log(LogError, reason);
        emit notice(false, reason);
        return;
    }

    switch (which) {
    case Pending::Ping:
        setBusy(false);
        log(LogOk, QStringLiteral("链路正常"));
        emit notice(true, QStringLiteral("链路正常"));
        break;
    case Pending::GetInfo: {
        DeviceInfo info;
        if (!parseInfo(payload, info)) {
            if (m_upgrading)
                failUpgrade(QStringLiteral("GET_INFO 应答长度不对"));
            else {
                setBusy(false);
                log(LogError, QStringLiteral("GET_INFO 应答长度不对"));
                emit notice(false, QStringLiteral("GET_INFO 应答长度不对"));
            }
            break;
        }
        if (info.maxPayload >= 4 && info.maxPayload <= IapCodec::kMaxDataPayload)
            m_maxPayload = info.maxPayload;
        else
            m_maxPayload = IapCodec::kMaxDataPayload;
        emit deviceInfoChanged(info);
        log(LogOk, QString("设备  协议 v%1  Boot %2  App %3  容量 %4 字节  载荷 %5  有效 %6  大小 %7  CRC32 %8")
                           .arg(info.protoVer)
                           .arg(hex32(info.bootVer))
                           .arg(hex32(info.appBase))
                           .arg(info.appMax)
                           .arg(info.maxPayload)
                           .arg(info.appValid ? QStringLiteral("是") : QStringLiteral("否"))
                           .arg(info.appSize)
                           .arg(hex32(info.appCrc32)));
        if (m_postUpgradeInfo) {
            m_postUpgradeInfo = false;
            m_upgrading = false;
            finishUpgrade(true, QString("升级完成，用时 %1 秒").arg(m_upgradeTimer.elapsed() / 1000.0, 0, 'f', 1));
            break;
        }
        if (!m_upgrading) {
            setBusy(false);
            emit notice(true, QStringLiteral("已读取设备信息"));
            break;
        }
        if (info.protoVer != 1) {
            failUpgrade(QString("协议版本是 %1，上位机只支持版本 1").arg(info.protoVer));
            break;
        }
        if (info.appMax == 0 || quint32(m_image.size()) > info.appMax) {
            failUpgrade(QString("固件 %1 字节，设备 App 区 %2 字节，已取消，未擦除")
                                .arg(m_image.size())
                                .arg(info.appMax));
            break;
        }
        QByteArray startPayload;
        IapCodec::appendU32(startPayload, quint32(m_image.size()));
        IapCodec::appendU32(startPayload, m_imageCrc);
        m_needAbortOnFail = true;
        log(LogInfo, QStringLiteral("START 会擦除被固件覆盖的扇区，请等待"));
        emit progressChanged(0, m_image.size(), QStringLiteral("正在擦除 App 扇区"), true);
        sendNew(IapCmd::Start, startPayload, Pending::Start);
        break;
    }
    case Pending::Start:
        m_deviceSession = true;
        m_offset = 0;
        m_resyncs = 0;
        log(LogOk, QStringLiteral("擦除完成，开始传输"));
        sendNextData();
        break;
    case Pending::Data: {
        if (payload.size() < 5) {
            failUpgrade(QStringLiteral("DATA 应答缺少期望偏移"));
            break;
        }
        const quint32 expect = IapCodec::readU32(payload, 1);
        const quint32 want = m_sentOffset + m_sentLength;
        if (status == 0x05 || expect != want) {
            if (expect > quint32(m_image.size()) || ++m_resyncs > 8) {
                failUpgrade(QString("偏移错误，设备期望 %1").arg(hex32(expect)));
                break;
            }
            log(LogWarn, QString("偏移不一致，从 %1 继续（第 %2 次）").arg(hex32(expect)).arg(m_resyncs));
            m_offset = expect;
            sendNextData();
            break;
        }
        m_resyncs = 0;
        m_offset = expect;
        if (m_offset >= m_image.size())
            sendEndCommand();
        else
            sendNextData();
        break;
    }
    case Pending::End:
        m_deviceSession = false;
        m_needAbortOnFail = false;
        log(LogOk, QStringLiteral("镜像 CRC 与向量表校验通过"));
        emit progressChanged(m_image.size(), m_image.size(), QStringLiteral("校验通过"), false);
        if (m_autoJump) {
            emit progressChanged(m_image.size(), m_image.size(), QStringLiteral("正在请求跳转"), false);
            sendNew(IapCmd::Jump, {}, Pending::Jump);
        } else {
            m_postUpgradeInfo = true;
            sendNew(IapCmd::GetInfo, {}, Pending::GetInfo);
        }
        break;
    case Pending::Jump:
        m_deviceSession = false;
        m_needAbortOnFail = false;
        if (m_upgrading) {
            m_upgrading = false;
            finishUpgrade(true, QString("升级完成，设备约 50ms 后复位并跳转 App，用时 %1 秒")
                                      .arg(m_upgradeTimer.elapsed() / 1000.0, 0, 'f', 1));
        } else {
            setBusy(false);
            log(LogOk, QStringLiteral("JUMP 已应答，设备即将复位"));
            emit notice(true, QStringLiteral("设备即将复位并跳转 App"));
        }
        break;
    case Pending::Abort: {
        m_deviceSession = false;
        m_needAbortOnFail = false;
        m_inFailAbort = false;
        const bool userCancel = m_userCancel;
        const QString stored = m_failureReason;
        m_userCancel = false;
        m_failureReason.clear();
        setBusy(false);
        if (userCancel) {
            log(LogWarn, QStringLiteral("升级已中止，已擦写的 App 视为无效"));
            emit upgradeFinished(false, QStringLiteral("已中止"));
        } else if (!stored.isEmpty()) {
            log(LogWarn, QStringLiteral("已发送 ABORT"));
            emit upgradeFinished(false, stored);
        } else {
            log(LogOk, QStringLiteral("ABORT 完成"));
            emit notice(true, QStringLiteral("ABORT 完成"));
        }
        break;
    }
    case Pending::None:
        break;
    }
}

void IapClient::sendEndCommand()
{
    log(LogInfo, QStringLiteral("固件已发完，请求 END 校验"));
    emit progressChanged(m_image.size(), m_image.size(), QStringLiteral("正在校验镜像"), false);
    sendNew(IapCmd::End, {}, Pending::End);
}

void IapClient::sendNextData()
{
    if (m_offset >= m_image.size()) {
        sendEndCommand();
        return;
    }
    const int length = chunkLength();
    if (length <= 0) {
        failUpgrade(QStringLiteral("设备单包长度无法按 4 字节对齐"));
        return;
    }
    QByteArray payload;
    IapCodec::appendU32(payload, quint32(m_offset));
    payload.append(m_image.mid(int(m_offset), length));
    m_sentOffset = quint32(m_offset);
    m_sentLength = quint32(length);
    const qint64 ms = qMax(qint64(1), m_upgradeTimer.elapsed());
    const double speed = (double(m_offset) / 1024.0) / (double(ms) / 1000.0);
    emit progressChanged(m_offset, m_image.size(),
                         QString("已写入  %1 / %2 字节  %3 KB/s")
                                 .arg(m_offset)
                                 .arg(m_image.size())
                                 .arg(speed, 0, 'f', 1),
                         false);
    sendNew(IapCmd::Data, payload, Pending::Data);
}

int IapClient::chunkLength() const
{
    const qint64 remain = m_image.size() - m_offset;
    int limit = m_maxPayload;
    if (limit < 1 || limit > IapCodec::kMaxDataPayload)
        limit = IapCodec::kMaxDataPayload;
    if (remain <= limit)
        return int(remain);
    const int aligned = limit - (limit % 4);
    if (aligned < 4)
        return -1;
    return aligned;
}

void IapClient::handleBusy()
{
    if (++m_busyRetries > 40) {
        failCurrent(QStringLiteral("设备一直处于擦除忙"));
        return;
    }
    log(LogWarn, QString("设备忙，300ms 后用新序号重试（%1）").arg(m_busyRetries));
    const int epoch = m_epoch;
    const IapCmd cmd = IapCmd(m_waitCmd);
    const QByteArray payload = m_lastPayload;
    const Pending pending = m_pending;
    QTimer::singleShot(300, this, [this, epoch, cmd, payload, pending]() {
        if (epoch != m_epoch || m_pending == Pending::None)
            return;
        sendNew(cmd, payload, pending, false);
    });
}

void IapClient::onTimeout()
{
    if (m_pending == Pending::None)
        return;
    if (m_retries < maxRetries(m_pending)) {
        ++m_retries;
        log(LogWarn, QString("应答超时，按序号 %1 重发（第 %2 次）").arg(m_waitSeq).arg(m_retries));
        if (!writeFrame(true)) {
            failCurrent(QStringLiteral("串口发送失败"));
            return;
        }
        m_timer.start(timeoutFor(m_pending));
        return;
    }
    failCurrent(QString("%1 应答超时").arg(IapCodec::commandTitle(m_waitCmd)));
}

void IapClient::failCurrent(const QString &reason)
{
    m_timer.stop();
    ++m_epoch;
    m_pending = Pending::None;
    if (m_inFailAbort) {
        m_inFailAbort = false;
        m_deviceSession = false;
        m_needAbortOnFail = false;
        m_upgrading = false;
        setBusy(false);
        log(LogError, reason);
        const QString stored = m_failureReason.isEmpty() ? reason : m_failureReason;
        m_failureReason.clear();
        emit upgradeFinished(false, stored);
        return;
    }
    if (m_upgrading || m_deviceSession || m_needAbortOnFail) {
        failUpgrade(reason);
        return;
    }
    setBusy(false);
    log(LogError, reason);
    emit notice(false, reason);
}

void IapClient::failUpgrade(const QString &reason)
{
    log(LogError, reason);
    const bool canAbort = (m_deviceSession || m_needAbortOnFail) && m_port->isOpen();
    m_upgrading = false;
    m_postUpgradeInfo = false;
    if (canAbort) {
        m_inFailAbort = true;
        m_failureReason = reason;
        m_deviceSession = false;
        m_needAbortOnFail = false;
        log(LogWarn, QStringLiteral("发送 ABORT 结束设备上的升级会话"));
        sendNew(IapCmd::Abort, {}, Pending::Abort);
        return;
    }
    m_deviceSession = false;
    m_needAbortOnFail = false;
    setBusy(false);
    emit upgradeFinished(false, reason);
}

void IapClient::finishUpgrade(bool ok, const QString &message)
{
    m_upgrading = false;
    m_deviceSession = false;
    m_needAbortOnFail = false;
    setBusy(false);
    log(ok ? LogOk : LogError, message);
    emit upgradeFinished(ok, message);
}

void IapClient::onPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError || m_handlingError)
        return;
    const QString text = m_port->errorString();
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError
        || error == QSerialPort::PermissionError) {
        m_handlingError = true;
        log(LogError, QString("串口异常：%1").arg(text));
        closePort();
        m_handlingError = false;
    }
}

int IapClient::timeoutFor(Pending pending) const
{
    return pending == Pending::Start ? 20000 : 500;
}

int IapClient::maxRetries(Pending pending) const
{
    return pending == Pending::Start ? 1 : 3;
}

bool IapClient::parseInfo(const QByteArray &payload, DeviceInfo &info) const
{
    if (payload.size() < 23)
        return false;
    info.protoVer = quint8(payload.at(1));
    info.bootVer = IapCodec::readU16(payload, 2);
    info.appBase = IapCodec::readU32(payload, 4);
    info.appMax = IapCodec::readU32(payload, 8);
    info.maxPayload = IapCodec::readU16(payload, 12);
    info.appValid = quint8(payload.at(14));
    info.appSize = IapCodec::readU32(payload, 15);
    info.appCrc32 = IapCodec::readU32(payload, 19);
    return true;
}

void IapClient::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged(busy);
}

void IapClient::log(LogLevel level, const QString &text) const
{
    emit const_cast<IapClient *>(this)->logMessage(int(level), text);
}

void IapClient::logTx() const
{
    QString title = QString("发送 %1  序号=%2  %3 字节")
                            .arg(IapCodec::commandTitle(m_waitCmd))
                            .arg(m_waitSeq)
                            .arg(m_lastFrame.size());
    if (m_waitCmd == quint8(IapCmd::Data))
        title += QString("  偏移=%1  长度=%2").arg(hex32(m_sentOffset)).arg(m_sentLength);
    if (m_waitCmd == quint8(IapCmd::Start))
        title += QString("  大小=%1  CRC32=%2").arg(m_image.size()).arg(hex32(m_imageCrc));
    log(LogTx, title);
    if (m_verbose || m_lastFrame.size() <= 40)
        log(LogTx, QString("  %1").arg(IapCodec::toHex(m_lastFrame)));
}

QString IapClient::hex32(quint32 value) const
{
    return QString("0x%1").arg(QString("%1").arg(value, 8, 16, QChar('0')).toUpper());
}
