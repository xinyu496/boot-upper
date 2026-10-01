/****************************************************************************
** Meta object code from reading C++ file 'iapclient.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../iapclient.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'iapclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9IapClientE_t {};
} // unnamed namespace

template <> constexpr inline auto IapClient::qt_create_metaobjectdata<qt_meta_tag_ZN9IapClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "IapClient",
        "logMessage",
        "",
        "level",
        "text",
        "portOpenChanged",
        "open",
        "busyChanged",
        "busy",
        "deviceInfoChanged",
        "DeviceInfo",
        "info",
        "progressChanged",
        "sent",
        "total",
        "indeterminate",
        "upgradeFinished",
        "ok",
        "message",
        "notice",
        "ping",
        "queryInfo",
        "startUpgrade",
        "image",
        "autoJump",
        "abortSession",
        "jumpToApp",
        "onReadyRead",
        "onTimeout",
        "onPortError",
        "QSerialPort::SerialPortError",
        "error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'logMessage'
        QtMocHelpers::SignalData<void(int, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'portOpenChanged'
        QtMocHelpers::SignalData<void(bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'busyChanged'
        QtMocHelpers::SignalData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 8 },
        }}),
        // Signal 'deviceInfoChanged'
        QtMocHelpers::SignalData<void(const DeviceInfo &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'progressChanged'
        QtMocHelpers::SignalData<void(qint64, qint64, const QString &, bool)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 13 }, { QMetaType::LongLong, 14 }, { QMetaType::QString, 4 }, { QMetaType::Bool, 15 },
        }}),
        // Signal 'upgradeFinished'
        QtMocHelpers::SignalData<void(bool, const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 17 }, { QMetaType::QString, 18 },
        }}),
        // Signal 'notice'
        QtMocHelpers::SignalData<void(bool, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 17 }, { QMetaType::QString, 18 },
        }}),
        // Slot 'ping'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'queryInfo'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'startUpgrade'
        QtMocHelpers::SlotData<void(const QByteArray &, bool)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QByteArray, 23 }, { QMetaType::Bool, 24 },
        }}),
        // Slot 'abortSession'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'jumpToApp'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTimeout'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPortError'
        QtMocHelpers::SlotData<void(QSerialPort::SerialPortError)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 30, 31 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<IapClient, qt_meta_tag_ZN9IapClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject IapClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IapClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IapClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9IapClientE_t>.metaTypes,
    nullptr
} };

void IapClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<IapClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->portOpenChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->busyChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->deviceInfoChanged((*reinterpret_cast<std::add_pointer_t<DeviceInfo>>(_a[1]))); break;
        case 4: _t->progressChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 5: _t->upgradeFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->notice((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->ping(); break;
        case 8: _t->queryInfo(); break;
        case 9: _t->startUpgrade((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 10: _t->abortSession(); break;
        case 11: _t->jumpToApp(); break;
        case 12: _t->onReadyRead(); break;
        case 13: _t->onTimeout(); break;
        case 14: _t->onPortError((*reinterpret_cast<std::add_pointer_t<QSerialPort::SerialPortError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< DeviceInfo >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(int , const QString & )>(_a, &IapClient::logMessage, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(bool )>(_a, &IapClient::portOpenChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(bool )>(_a, &IapClient::busyChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(const DeviceInfo & )>(_a, &IapClient::deviceInfoChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(qint64 , qint64 , const QString & , bool )>(_a, &IapClient::progressChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(bool , const QString & )>(_a, &IapClient::upgradeFinished, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (IapClient::*)(bool , const QString & )>(_a, &IapClient::notice, 6))
            return;
    }
}

const QMetaObject *IapClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IapClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IapClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int IapClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void IapClient::logMessage(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void IapClient::portOpenChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void IapClient::busyChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void IapClient::deviceInfoChanged(const DeviceInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void IapClient::progressChanged(qint64 _t1, qint64 _t2, const QString & _t3, bool _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 5
void IapClient::upgradeFinished(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void IapClient::notice(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}
QT_WARNING_POP
