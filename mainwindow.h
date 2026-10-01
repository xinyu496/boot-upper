#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "iapclient.h"

#include <QMainWindow>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void buildUi();
    void refreshPorts();
    void loadSettings();
    void saveSettings() const;
    void updateActions();
    void loadImage(const QString &path, bool warn);
    void appendLog(int level, const QString &text);
    void showDeviceInfo(const DeviceInfo &info);
    void onProgress(qint64 sent, qint64 total, const QString &text, bool indeterminate);
    void onUpgradeFinished(bool ok, const QString &message);
    void onNotice(bool ok, const QString &message);
    void togglePort();
    void browseFirmware();
    void startUpgrade();
    void showHelp();

    IapClient *m_client = nullptr;
    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QPushButton *m_openBtn = nullptr;
    QLineEdit *m_fileEdit = nullptr;
    QPushButton *m_browseBtn = nullptr;
    QLabel *m_fileMetaLabel = nullptr;
    QLabel *m_vectorLabel = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_progressLabel = nullptr;
    QPushButton *m_pingBtn = nullptr;
    QPushButton *m_infoBtn = nullptr;
    QPushButton *m_upgradeBtn = nullptr;
    QPushButton *m_abortBtn = nullptr;
    QPushButton *m_jumpBtn = nullptr;
    QCheckBox *m_autoJump = nullptr;
    QCheckBox *m_verbose = nullptr;
    QTextEdit *m_log = nullptr;
    QLabel *m_protoLabel = nullptr;
    QLabel *m_bootLabel = nullptr;
    QLabel *m_baseLabel = nullptr;
    QLabel *m_maxLabel = nullptr;
    QLabel *m_payloadLabel = nullptr;
    QLabel *m_validLabel = nullptr;
    QLabel *m_appSizeLabel = nullptr;
    QLabel *m_appCrcLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QByteArray m_image;
};

#endif // MAINWINDOW_H
