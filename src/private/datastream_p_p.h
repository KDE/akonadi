/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_PROTOCOL_DATASTREAM_P_P_H
#define AKONADI_PROTOCOL_DATASTREAM_P_P_H

#include <type_traits>

#include "akonadiprivate_export.h"
#include "protocol_exception_p.h"

#include <QByteArray>
#include <QIODevice>
#include <QTimeZone>

namespace Akonadi
{
namespace Protocol
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

    int waitTimeout() const;
    void setWaitTimeout(int timeout);

    void flush();

    template<typename T>
    inline typename std::enable_if<std::is_integral<T>::value, DataStream>::type
    &operator<<(T val);
    template<typename T>
    inline typename std::enable_if<std::is_enum<T>::value, DataStream>::type
    &operator<<(T val);

    inline DataStream &operator<<(const QString &str);
    inline DataStream &operator<<(const QByteArray &data);
    inline DataStream &operator<<(const QDateTime &dt);

    template<typename T>
    inline typename std::enable_if<std::is_integral<T>::value, DataStream>::type
    &operator>>(T &val);
    template<typename T>
    inline typename std::enable_if<std::is_enum<T>::value, DataStream>::type
    &operator>>(T &val);
    inline DataStream &operator>>(QString &str);
    inline DataStream &operator>>(QByteArray &data);
    inline DataStream &operator>>(QDateTime &dt);

    void writeRawData(const char *data, int len);
    void writeBytes(const char *bytes, int len);

    int readRawData(char *buffer, int len);

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
    int mWaitTimeout;
};

template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, DataStream>::type
&DataStream::operator<<(T val)
{
    checkDevice();
    writeRawData((char *)&val, sizeof(T));
    return *this;
}

template<typename T>
inline typename std::enable_if<std::is_enum<T>::value, DataStream>::type
&DataStream::operator<<(T val)
{
    return *this << (typename std::underlying_type<T>::type) val;
}

inline DataStream &DataStream::operator<<(const QString &str)
{
    if (str.isNull()) {
        *this << (quint32) 0xffffffff;
    } else {
        writeBytes(reinterpret_cast<const char *>(str.unicode()), sizeof(QChar) * str.length());
    }
    return *this;
}

inline DataStream &DataStream::operator<<(const QByteArray &data)
{
    if (data.isNull()) {
        *this << (quint32) 0xffffffff;
    } else {
        writeBytes(data.constData(), data.size());
    }
    return *this;
}

inline DataStream &DataStream::operator<<(const QDateTime &dt)
{
    *this << dt.date().toJulianDay()
          << dt.time().msecsSinceStartOfDay()
          << dt.timeSpec();
    if (dt.timeSpec() == Qt::OffsetFromUTC) {
        *this << dt.offsetFromUtc();
    } else if (dt.timeSpec() == Qt::TimeZone) {
        *this << dt.timeZone().id();
    }
    return *this;
}

template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, DataStream>::type
&DataStream::operator>>(T &val)
{
    checkDevice();

    waitForData(sizeof(T));

    if (mDev->read((char *)&val, sizeof(T)) != sizeof(T)) {
        throw Akonadi::ProtocolException("Failed to read enough data from stream");
    }
    return *this;
}

template<typename T>
inline typename std::enable_if<std::is_enum<T>::value, DataStream>::type
&DataStream::operator>>(T &val)
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
        str = QString(QLatin1String(""));
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
        if (readRawData(reinterpret_cast<char *>(str.data()) + allocated * sizeof(QChar),
                        blockSize * sizeof(QChar)) != int(blockSize * sizeof(QChar))) {
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
    int mds;
    Qt::TimeSpec spec;

    *this >> jd
          >> mds
          >> spec;
    date = QDate::fromJulianDay(jd);
    time = QTime::fromMSecsSinceStartOfDay(mds);
    if (spec == Qt::OffsetFromUTC) {
        int offset = 0;
        *this >> offset;
        dt = QDateTime(date, time, spec, offset);
    } else if (spec == Qt::TimeZone) {
        QByteArray id;
        *this >> id;
        dt = QDateTime(date, time, QTimeZone(id));
    } else {
        dt = QDateTime(date, time, spec);
    }

    return *this;
}

} // namespace Protocol
} // namespace Akonadi

// Inline functions

template<typename T>
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, QFlags<T> flags)
{
    return stream << static_cast<typename QFlags<T>::Int>(flags);
}

template<typename T>
inline Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, QFlags<T> &flags)
{
    stream >> reinterpret_cast<typename QFlags<T>::Int&>(flags);
    return stream;
}

// Generic streaming for all Qt value-based containers (as well as std containers that
// implement operator<< for appending)
template<typename T, template<typename> class Container>
//typename std::enable_if<is_compatible_value_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Container<T> &list)
{
    stream << (quint32) list.size();
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

inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const QStringList &list)
{
    return stream << static_cast<QList<QString>>(list);
}

inline Akonadi::Protocol::DataStream &operator>>(Akonadi::Protocol::DataStream &stream, QStringList &list)
{
    return stream >> static_cast<QList<QString>&>(list);
}

namespace Akonadi
{
namespace Protocol
{
namespace Private
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
} // namespace Private
} // namespace Protocol
} // namespace Akonadi

// Generic streaming for all Qt dictionary-based containers
template<typename Key, typename Value, template<typename, typename> class Container>
// typename std::enable_if<is_compatible_dictionary_container<Container>::value, Akonadi::Protocol::DataStream>::type
inline Akonadi::Protocol::DataStream &operator<<(Akonadi::Protocol::DataStream &stream, const Container<Key, Value> &map)
{
    stream << (quint32) map.size();
    auto iter = map.cend(), begin = map.cbegin();
    while (iter != begin) {
        --iter;
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
        map.insertMulti(key, value);
    }
    return stream;
}

#endif // AKONADI_PROTOCOL_DATASTREAM_P_P_H
