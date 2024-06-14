/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <chrono>
#include <qnamespace.h>
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

    inline DataStream &operator<<(const QString &str);
    inline DataStream &operator<<(const QByteArray &data);
    inline DataStream &operator<<(const QDateTime &dt);

    template<typename T>
        requires(std::is_integral_v<T>)
    inline DataStream &operator>>(T &val);
    template<typename T>
        requires(std::is_enum_v<T>)
    inline DataStream &operator>>(T &val);
    inline DataStream &operator>>(QString &str);
    inline DataStream &operator>>(QByteArray &data);
    inline DataStream &operator>>(QDateTime &dt);

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

inline DataStream &DataStream::operator<<(const QString &str)
{
    if (str.isNull()) {
        *this << (quint32)0xffffffff;
    } else {
        writeBytes(reinterpret_cast<const char *>(str.unicode()), sizeof(QChar) * str.length());
    }
    return *this;
}

inline DataStream &DataStream::operator<<(const QByteArray &data)
{
    if (data.isNull()) {
        *this << (quint32)0xffffffff;
    } else {
        writeBytes(data.constData(), data.size());
    }
    return *this;
}

inline DataStream &DataStream::operator<<(const QDateTime &dt)
{
    *this << dt.date().toJulianDay() << dt.time().msecsSinceStartOfDay() << dt.timeSpec();
    if (dt.timeSpec() == Qt::OffsetFromUTC) {
        *this << dt.offsetFromUtc();
    } else if (dt.timeSpec() == Qt::TimeZone) {
        *this << dt.timeZone().id();
    }
    return *this;
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

inline DataStream &DataStream::operator>>(QString &str)
{
    str.clear();

    quint32 bytes = 0;
    *this >> bytes;
    if (bytes == 0xffffffff) {
        return *this;
    } else if (bytes == 0) {
        str = QString(QLatin1StringView(""));
        return *this;
    }

    if (bytes & 0x1) {
        str.clear();
        throw Akonadi::ProtocolException("Read corrupt data");
    }

    const quint32 step = 1024 * 1024;
    const quint32 len = bytes / 2;
    quint32 allocated = 0;

    while (allocated < len) {
        const int blockSize = qMin(step, len - allocated);
        waitForData(blockSize * sizeof(QChar));
        str.resize(allocated + blockSize);
        if (readRawData(reinterpret_cast<char *>(str.data()) + allocated * sizeof(QChar), blockSize * sizeof(QChar)) != int(blockSize * sizeof(QChar))) {
            throw Akonadi::ProtocolException("Failed to read enough data from stream");
        }
        allocated += blockSize;
    }

    return *this;
}

inline DataStream &DataStream::operator>>(QByteArray &data)
{
    data.clear();

    quint32 len = 0;
    *this >> len;
    if (len == 0xffffffff) {
        return *this;
    }

    const quint32 step = 1024 * 1024;
    quint32 allocated = 0;

    while (allocated < len) {
        const int blockSize = qMin(step, len - allocated);
        waitForData(blockSize);
        data.resize(allocated + blockSize);
        if (readRawData(data.data() + allocated, blockSize) != blockSize) {
            throw Akonadi::ProtocolException("Failed to read enough data from stream");
        }
        allocated += blockSize;
    }

    return *this;
}

inline DataStream &DataStream::operator>>(QDateTime &dt)
{
    QDate date;
    QTime time;
    qint64 jd;
    Qt::TimeSpec spec;
    int mds;
    QTimeZone timezone(QTimeZone::LocalTime);

    *this >> jd >> mds >> spec;
    date = QDate::fromJulianDay(jd);
    time = QTime::fromMSecsSinceStartOfDay(mds);
    switch (spec) {
    case Qt::UTC:
        timezone = QTimeZone::utc();
        break;
    case Qt::OffsetFromUTC: {
        int offset = 0;
        *this >> offset;
        timezone = QTimeZone::fromSecondsAheadOfUtc(offset);
        break;
    }
    case Qt::LocalTime:
        break;
    case Qt::TimeZone: {
        QByteArray id;
        *this >> id;
        timezone = QTimeZone(id);
    }
    }

    dt = QDateTime(date, time, timezone);
    return *this;
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

namespace Akonadi::Protocol::Private
{

template<typename Key, typename Value, template<typename, typename> class Container>
inline void container_reserve(Container<Key, Value> &container, int size)
{
    container.reserve(size);
}

template<typename Key, typename Value>
inline void container_reserve(QMap<Key, Value> &, int)
{
    // noop
}

} // namespace Akonadi::Protocol::Private

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
    Akonadi::Protocol::Private::container_reserve(map, size);
    for (quint32 i = 0; i < size; ++i) {
        Key key;
        Value value;
        stream >> key >> value;
        map.insert(key, value);
    }
    return stream;
}
