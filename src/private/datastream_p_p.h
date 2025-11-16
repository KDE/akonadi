/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <chrono>
#include <shared/aktraits.h>
#include <type_traits>

#include "akonadiprivate_export.h"
#include "protocol_exception_p.h"

#include <QByteArray>
#include <QIODevice>
#include <QTimeZone>

namespace Akonadi::Protocol
{

class AKONADIPRIVATE_EXPORT DataStream
{
public:
    explicit DataStream();
    explicit DataStream(QIODevice *device);
    ~DataStream();

    static void waitForData(QIODevice *device, int timeoutMs);

    QIODevice *device() const;
    void setDevice(QIODevice *device);

    std::chrono::milliseconds waitTimeout() const;
    void setWaitTimeout(std::chrono::milliseconds timeout);

    void flush();

    template<typename T>
        requires(std::is_integral_v<T>)
    inline DataStream &operator<<(T val);
    template<typename T>
        requires(std::is_enum_v<T>)
    inline DataStream &operator<<(T val);

    DataStream &operator<<(const QString &str);
    DataStream &operator<<(const QByteArray &data);
    DataStream &operator<<(const QDateTime &dt);

    template<typename T>
        requires(std::is_integral_v<T>)
    inline DataStream &operator>>(T &val);
    template<typename T>
        requires(std::is_enum_v<T>)
    inline DataStream &operator>>(T &val);
    DataStream &operator>>(QString &str);
    DataStream &operator>>(QByteArray &data);
    DataStream &operator>>(QDateTime &dt);

    void writeRawData(const char *data, qsizetype len);
    void writeBytes(const char *bytes, qsizetype len);

    [[nodiscard]] qint64 readRawData(char *buffer, qint64 len);

    void waitForData(quint32 size);

private:
    Q_DISABLE_COPY(DataStream)

    inline void checkDevice() const
    {
        if (Q_UNLIKELY(!mDev)) {
            throw ProtocolException("Device does not exist");
        }
    }

    QIODevice *mDev;
    QByteArray mWriteBuffer;
    std::chrono::milliseconds mWaitTimeout = std::chrono::seconds{30};
};

template<typename T>
    requires(std::is_integral_v<T>)
inline DataStream &DataStream::operator<<(T val)
{
    checkDevice();
    writeRawData((char *)&val, sizeof(T));
    return *this;
}

template<typename T>
    requires(std::is_enum_v<T>)
inline DataStream &DataStream::operator<<(T val)
{
    return *this << (typename std::underlying_type<T>::type)val;
}

template<typename T>
    requires(std::is_integral_v<T>)
inline DataStream &DataStream::operator>>(T &val)
{
    checkDevice();

    waitForData(sizeof(T));

    if (mDev->read((char *)&val, sizeof(T)) != sizeof(T)) {
        throw Akonadi::ProtocolException("Failed to read enough data from stream");
    }
    return *this;
}

template<typename T>
    requires(std::is_enum_v<T>)
inline DataStream &DataStream::operator>>(T &val)
{
    return *this >> reinterpret_cast<typename std::underlying_type<T>::type &>(val);
}

} // namespace Akonadi::Protocol

// Inline functions

template<typename T>
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, QFlags<T> flags)
{
    return stream << static_cast<typename QFlags<T>::Int>(flags);
}

template<typename T>
inline Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, QFlags<T> &flags)
{
    stream >> reinterpret_cast<typename QFlags<T>::Int &>(flags);
    return stream;
}

// Generic streaming for all Qt value-based containers (as well as std containers that
// implement operator<< for appending)
template<typename T, template<typename> class Container>
// typename std::enable_if<is_compatible_value_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Container<T> &list)
{
    stream << (quint32)list.size();
    for (auto iter = list.cbegin(), end = list.cend(); iter != end; ++iter) {
        stream << *iter;
    }
    return stream;
}

template<typename T, template<typename> class Container>
// //typename std::enable_if<is_compatible_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Container<T> &list)
{
    list.clear();
    quint32 size = 0;
    stream >> size;
    list.reserve(size);
    for (quint32 i = 0; i < size; ++i) {
        T t;
        stream >> t;
        list << t;
    }
    return stream;
}

// Generic streaming for all Qt dictionary-based containers
template<typename Key, typename Value, template<typename, typename> class Container>
// typename std::enable_if<is_compatible_dictionary_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Container<Key, Value> &map)
{
    stream << (quint32)map.size();
    for (auto iter = map.cbegin(), end = map.cend(); iter != end; ++iter) {
        stream << iter.key() << iter.value();
    }
    return stream;
}

template<typename Key, typename Value, template<typename, typename> class Container>
// typename std::enable_if<is_compatible_dictionary_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, Container<Key, Value> &map)
{
    map.clear();
    quint32 size = 0;
    stream >> size;
    if constexpr (AkTraits::ReservableContainer<Container<Key, Value>>) {
        map.reserve(size);
    }
    for (quint32 i = 0; i < size; ++i) {
        Key key;
        Value value;
        stream >> key >> value;
        map.insert(key, value);
    }
    return stream;
}
