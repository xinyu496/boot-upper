#ifndef IAPCLIENT_H
#define IAPCLIENT_H

#include "iapcodec.h"

#include <QElapsedTimer>
#include <QObject>
#include <QSerialPort>
#include <QTimer>

enum LogLevel {
    LogInfo = 0,
    LogTx,
    LogRx,
    LogOk,
    LogWarn,
    LogError
};

struct DeviceInfo {
    quint8 protoVer = 0;
    quint16 bootVer = 0;
    quint32 appBase = 0;
    quint32 appMax = 0;
    quint16 maxPayload = 0;
    quint8 appValid = 0;
    quint32 appSize = 0;
    quint32 appCrc32 = 0;
};

Q_DECLARE_METATYPE(DeviceInfo)

class IapClient : public QObject
{
    Q_OBJECT

public:
    explicit IapClient(QObject *parent = nullptr);

    bool isOpen() const;
    bool isBusy() const;
    QString portName() const;

    QString openPort(const QString &name, int baud);
    void closePort();
    void setVerboseHex(bool enabled);

public slots:
    void ping();
    void queryInfo();
    void startUpgrade(const QByteArray &image, bool autoJump);
    void abortSession();
    void jumpToApp();

signals:
    void logMessage(int level, const QString &text);
    void portOpenChanged(bool open);
    void busyChanged(bool busy);
    void deviceInfoChanged(const DeviceInfo &info);
    void progressChanged(qint64 sent, qint64 total, const QString &text, bool indeterminate);
    void upgradeFinished(bool ok, const QString &message);
    void notice(bool ok, const QString &message);

private slots:
    void onReadyRead();
    void onTimeout();
    void onPortError(QSerialPort::SerialPortError error);

private:
    enum class Pending {
        None,
        Ping,
        GetInfo,
        Start,
        Data,
        End,
        Abort,
        Jump
    };

    void sendNew(IapCmd cmd, const QByteArray &payload, Pending pending, bool resetBusyCount = true);
    void sendNextData();
    void sendEndCommand();
    bool writeFrame(bool retry);
    void failCurrent(const QString &reason);
    void failUpgrade(const QString &reason);
    void handleFrame(const IapFrame &frame);
    void handleReply(Pending which, quint8 status, const QByteArray &payload);
    void handleBusy();
    void finishUpgrade(bool ok, const QString &message);
    int chunkLength() const;
    int timeoutFor(Pending pending) const;
    int maxRetries(Pending pending) const;
    bool parseInfo(const QByteArray &payload, DeviceInfo &info) const;
    void setBusy(bool busy);
    void log(LogLevel level, const QString &text) const;
    void logTx() const;
    QString hex32(quint32 value) const;

    QSerialPort *m_port = nullptr;
    QTimer m_timer;
    QElapsedTimer m_upgradeTimer;
    QByteArray m_rx;
    QByteArray m_lastFrame;
    QByteArray m_lastPayload;
    QByteArray m_image;

    Pending m_pending = Pending::None;
    quint16 m_seq = 0;
    quint16 m_waitSeq = 0;
    quint8 m_waitCmd = 0;
    int m_retries = 0;
    int m_busyRetries = 0;
    int m_resyncs = 0;
    int m_epoch = 0;
    int m_maxPayload = IapCodec::kMaxDataPayload;
    bool m_busy = false;
    bool m_verbose = false;
    bool m_upgrading = false;
    bool m_autoJump = true;
    bool m_deviceSession = false;
    bool m_needAbortOnFail = false;
    bool m_inFailAbort = false;
    bool m_userCancel = false;
    bool m_postUpgradeInfo = false;
    bool m_handlingError = false;
    quint32 m_imageCrc = 0;
    quint32 m_sentOffset = 0;
    quint32 m_sentLength = 0;
    qint64 m_offset = 0;
    QString m_failureReason;
};

#endif // IAPCLIENT_H
