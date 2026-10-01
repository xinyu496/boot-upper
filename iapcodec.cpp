#include "iapcodec.h"

#include <cstring>

quint16 IapCodec::crc16Modbus(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= quint8(data.at(i));
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001)
                crc = quint16((crc >> 1) ^ 0xA001);
            else
                crc = quint16(crc >> 1);
        }
    }
    return crc;
}

quint32 IapCodec::crc32IsoHdlc(const QByteArray &data)
{
    quint32 crc = 0xFFFFFFFFu;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= quint8(data.at(i));
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x00000001u)
                crc = (crc >> 1) ^ 0xEDB88320u;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

void IapCodec::appendU16(QByteArray &out, quint16 value)
{
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
}

void IapCodec::appendU32(QByteArray &out, quint32 value)
{
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 24) & 0xFF));
}

quint16 IapCodec::readU16(const QByteArray &in, int offset)
{
    if (offset < 0 || offset + 1 >= in.size())
        return 0;
    return quint16(quint8(in.at(offset)) | (quint16(quint8(in.at(offset + 1))) << 8));
}

quint32 IapCodec::readU32(const QByteArray &in, int offset)
{
    if (offset < 0 || offset + 3 >= in.size())
        return 0;
    return quint32(quint8(in.at(offset)))
            | (quint32(quint8(in.at(offset + 1))) << 8)
            | (quint32(quint8(in.at(offset + 2))) << 16)
            | (quint32(quint8(in.at(offset + 3))) << 24);
}

QByteArray IapCodec::encode(quint16 seq, quint8 cmd, const QByteArray &payload)
{
    const int total = kMinFrame + payload.size();
    if (total < kMinFrame || total > kMaxFrame || seq == 0)
        return {};

    QByteArray frame(total, '\0');
    frame[0] = char(kHead);
    frame[1] = char(total & 0xFF);
    frame[2] = char((total >> 8) & 0xFF);
    frame[3] = char(seq & 0xFF);
    frame[4] = char((seq >> 8) & 0xFF);
    frame[5] = char(cmd);
    if (!payload.isEmpty())
        memcpy(frame.data() + 6, payload.constData(), size_t(payload.size()));

    const quint16 crc = crc16Modbus(frame.left(total - 3));
    frame[total - 3] = char(crc & 0xFF);
    frame[total - 2] = char((crc >> 8) & 0xFF);
    frame[total - 1] = char(kTail);
    return frame;
}

IapTakeResult IapCodec::takeFrame(QByteArray &buffer)
{
    IapTakeResult result;
    const int head = buffer.indexOf(char(kHead));
    if (head < 0) {
        buffer.clear();
        result.status = IapTakeStatus::NeedMore;
        return result;
    }
    if (head > 0)
        buffer.remove(0, head);
    if (buffer.size() < 3) {
        result.status = IapTakeStatus::NeedMore;
        return result;
    }

    const int total = int(quint8(buffer.at(1)) | (quint16(quint8(buffer.at(2))) << 8));
    if (total < kMinFrame || total > kMaxFrame) {
        buffer.remove(0, 1);
        result.status = IapTakeStatus::BadFrame;
        result.reason = QString("非法长度 %1").arg(total);
        return result;
    }
    if (buffer.size() < total) {
        result.status = IapTakeStatus::NeedMore;
        return result;
    }
    if (quint8(buffer.at(total - 1)) != kTail) {
        buffer.remove(0, 1);
        result.status = IapTakeStatus::BadFrame;
        result.reason = "包尾不是 0x6F";
        return result;
    }

    const quint16 expect = crc16Modbus(buffer.left(total - 3));
    const quint16 got = quint16(quint8(buffer.at(total - 3))
                                 | (quint16(quint8(buffer.at(total - 2))) << 8));
    if (expect != got) {
        const QByteArray sample = buffer.left(qMin(total, 24));
        buffer.remove(0, 1);
        result.status = IapTakeStatus::BadFrame;
        result.reason = QString("CRC 不符，计算 0x%1，报文 0x%2，数据 %3")
                                .arg(QString("%1").arg(expect, 4, 16, QChar('0')).toUpper())
                                .arg(QString("%1").arg(got, 4, 16, QChar('0')).toUpper())
                                .arg(toHex(sample));
        return result;
    }

    result.status = IapTakeStatus::GotFrame;
    result.frame.seq = quint16(quint8(buffer.at(3)) | (quint16(quint8(buffer.at(4))) << 8));
    result.frame.cmd = quint8(buffer.at(5));
    result.frame.payload = buffer.mid(6, total - kMinFrame);
    result.frame.raw = buffer.left(total);
    buffer.remove(0, total);
    return result;
}

QString IapCodec::toHex(const QByteArray &data, int maxBytes)
{
    int count = data.size();
    bool trimmed = false;
    if (maxBytes >= 0 && count > maxBytes) {
        count = maxBytes;
        trimmed = true;
    }
    QString text;
    text.reserve(count * 3);
    for (int i = 0; i < count; ++i) {
        if (i)
            text += QLatin1Char(' ');
        text += QString("%1").arg(quint8(data.at(i)), 2, 16, QChar('0')).toUpper();
    }
    if (trimmed)
        text += QString(" ...(%1 字节)").arg(data.size());
    return text;
}

QString IapCodec::commandTitle(quint8 cmd)
{
    switch (cmd) {
    case quint8(IapCmd::Ping):
        return QStringLiteral("PING");
    case quint8(IapCmd::GetInfo):
        return QStringLiteral("GET_INFO");
    case quint8(IapCmd::Start):
        return QStringLiteral("START");
    case quint8(IapCmd::Data):
        return QStringLiteral("DATA");
    case quint8(IapCmd::End):
        return QStringLiteral("END");
    case quint8(IapCmd::Abort):
        return QStringLiteral("ABORT");
    case quint8(IapCmd::Jump):
        return QStringLiteral("JUMP");
    default:
        return QString("未知命令 0x%1").arg(QString("%1").arg(cmd, 2, 16, QChar('0')).toUpper());
    }
}

QString IapCodec::statusName(quint8 status)
{
    switch (status) {
    case 0x00:
        return QStringLiteral("成功");
    case 0x01:
        return QStringLiteral("CRC 错");
    case 0x02:
        return QStringLiteral("长度错");
    case 0x03:
        return QStringLiteral("未知命令");
    case 0x04:
        return QStringLiteral("序号错");
    case 0x05:
        return QStringLiteral("偏移错");
    case 0x06:
        return QStringLiteral("Flash 失败");
    case 0x07:
        return QStringLiteral("镜像 CRC 不符");
    case 0x08:
        return QStringLiteral("长度超出 App 区");
    case 0x09:
        return QStringLiteral("当前没有升级会话");
    case 0x0A:
        return QStringLiteral("忙（正在擦除）");
    case 0x0B:
        return QStringLiteral("App 无效不能跳转");
    default:
        return QString("未知状态 0x%1").arg(QString("%1").arg(status, 2, 16, QChar('0')).toUpper());
    }
}

QString IapCodec::selfCheckError()
{
    const QByteArray digits("123456789");
    if (crc16Modbus(digits) != 0x4B37)
        return QStringLiteral("CRC-16/MODBUS 自检失败");
    if (crc32IsoHdlc(digits) != 0xCBF43926u)
        return QStringLiteral("CRC-32/ISO-HDLC 自检失败");
    if (crc32IsoHdlc(QByteArray()) != 0)
        return QStringLiteral("CRC-32 空数据自检失败");

    QByteArray payload;
    payload.append(char(0x7E));
    payload.append(char(0x6F));
    payload.append(char(0x10));
    payload.append(char(0x00));
    const QByteArray frame = encode(1, quint8(IapCmd::Data), payload);
    if (frame.size() != kMinFrame + payload.size())
        return QStringLiteral("组帧长度错误");
    if (quint8(frame.at(0)) != kHead || quint8(frame.at(frame.size() - 1)) != kTail)
        return QStringLiteral("包头或包尾错误");
    const int declared = int(quint8(frame.at(1)) | (quint16(quint8(frame.at(2))) << 8));
    if (declared != frame.size())
        return QStringLiteral("长度字段错误");

    QByteArray stream;
    stream.append(char(0x00));
    stream.append(frame);
    stream.append(frame);
    stream.append(char(0x11));
    const IapTakeResult first = takeFrame(stream);
    const IapTakeResult second = takeFrame(stream);
    if (first.status != IapTakeStatus::GotFrame || second.status != IapTakeStatus::GotFrame)
        return QStringLiteral("粘包解析失败");
    if (first.frame.seq != 1 || first.frame.cmd != quint8(IapCmd::Data) || first.frame.payload != payload)
        return QStringLiteral("帧内容解析不一致");
    if (second.frame.payload != payload || stream.size() != 1 || quint8(stream.at(0)) != 0x11)
        return QStringLiteral("粘包残留错误");

    QByteArray bad = frame;
    bad[bad.size() - 3] = char(quint8(bad.at(bad.size() - 3)) ^ 0xFF);
    const IapTakeResult rejected = takeFrame(bad);
    if (rejected.status != IapTakeStatus::BadFrame)
        return QStringLiteral("错误 CRC 未被拒绝");
    if (encode(0, quint8(IapCmd::Ping), {}).isEmpty() == false)
        return QStringLiteral("序号 0 应被拒绝");
    return {};
}
