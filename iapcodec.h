#ifndef IAPCODEC_H
#define IAPCODEC_H

#include <QByteArray>
#include <QString>

// STM32F407 UART IAP frame:
// 0x7E | len u16 | seq u16 | cmd u8 + payload | crc16 u16 | 0x6F
// len is the whole frame size. Multi-byte fields are little-endian.
// CRC-16/MODBUS covers everything except the CRC itself and the tail.

enum class IapCmd : quint8 {
    Ping = 0x01,
    GetInfo = 0x02,
    Start = 0x10,
    Data = 0x11,
    End = 0x12,
    Abort = 0x13,
    Jump = 0x20
};

struct IapFrame {
    quint16 seq = 0;
    quint8 cmd = 0;
    QByteArray payload;
    QByteArray raw;
};

enum class IapTakeStatus {
    NeedMore,
    GotFrame,
    BadFrame
};

struct IapTakeResult {
    IapTakeStatus status = IapTakeStatus::NeedMore;
    IapFrame frame;
    QString reason;
};

class IapCodec
{
public:
    static constexpr int kMinFrame = 9;
    static constexpr int kMaxFrame = 269;
    static constexpr int kMaxDataPayload = 256;
    static constexpr quint8 kHead = 0x7E;
    static constexpr quint8 kTail = 0x6F;

    static quint16 crc16Modbus(const QByteArray &data);
    static quint32 crc32IsoHdlc(const QByteArray &data);

    static void appendU16(QByteArray &out, quint16 value);
    static void appendU32(QByteArray &out, quint32 value);
    static quint16 readU16(const QByteArray &in, int offset);
    static quint32 readU32(const QByteArray &in, int offset);

    static QByteArray encode(quint16 seq, quint8 cmd, const QByteArray &payload);
    static IapTakeResult takeFrame(QByteArray &buffer);

    static QString toHex(const QByteArray &data, int maxBytes = -1);
    static QString commandTitle(quint8 cmd);
    static QString statusName(quint8 status);
    static QString selfCheckError();
};

#endif // IAPCODEC_H
