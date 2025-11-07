/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "datastream_p_p.h"

#include <QLocalSocket>
#include <chrono>

#ifdef Q_OS_WIN
#include <QEventLoop>
#include <QLocalSocket>
#include <QTimer>
#endif

using namespace Akonadi;
using namespace Akonadi::Protocol;

DataStream::DataStream()
    : mDev(nullptr)
{
}

DataStream::DataStream(QIODevice *device)
    : mDev(device)
{
}

DataStream::~DataStream()
{
    // No flush() here. Throwing an exception in a destructor would go badly. The caller MUST call flush after writing.
}

void DataStream::flush()
{
    if (!mWriteBuffer.isEmpty()) {
        const int len = mWriteBuffer.size();
        int ret = mDev->write(mWriteBuffer);
        if (ret != len) {
            // TODO: Try to write data again unless ret is -1?
            throw ProtocolException("Failed to write all data");
        }
        mWriteBuffer.clear();
    }
}

void DataStream::waitForData(QIODevice *device, std::chrono::milliseconds timeout)
{
    auto ls = qobject_cast<QLocalSocket *>(device);
#ifdef Q_OS_WIN
    // Apparently readyRead() gets emitted sometimes even if there are no data
    // so we will re-enter the wait again immediately
    while (device->bytesAvailable() == 0) {
        if (ls && ls->state() != QLocalSocket::ConnectedState) {
            throw ProtocolTimeoutException();
        }

        QEventLoop loop;
        QObject::connect(device, &QIODevice::readyRead, &loop, &QEventLoop::quit);
        if (ls) {
            QObject::connect(ls, &QLocalSocket::stateChanged, &loop, &QEventLoop::quit);
        }
        bool timedOut = false;
        if (timeout > std::chrono::milliseconds::zero()) {
            QTimer::singleShot(timeout, &loop, [&]() {
                timedOut = true;
                loop.quit();
            });
        }
        loop.exec();
        if (timedOut) {
            throw ProtocolTimeoutException();
        }
        if (ls && ls->state() != QLocalSocket::ConnectedState) {
            throw ProtocolException("Socket not connected to server");
        }
    }
#else

    if (!device->waitForReadyRead(timeout.count())) {
        if (ls && ls->state() != QLocalSocket::ConnectedState) {
            throw ProtocolException("Socket not connected to server");
        } else {
            throw ProtocolTimeoutException();
        }
    }
#endif
}

QIODevice *DataStream::device() const
{
    return mDev;
}

void DataStream::setDevice(QIODevice *device)
{
    mDev = device;
}

std::chrono::milliseconds DataStream::waitTimeout() const
{
    return mWaitTimeout;
}
void DataStream::setWaitTimeout(std::chrono::milliseconds timeout)
{
    mWaitTimeout = timeout;
}

void DataStream::waitForData(quint32 size)
{
    checkDevice();

    while (mDev->bytesAvailable() < size) {
        waitForData(mDev, mWaitTimeout);
    }
}

void DataStream::writeRawData(const char *data, qsizetype len)
{
    checkDevice();

    mWriteBuffer += QByteArray::fromRawData(data, len);
}

void DataStream::writeBytes(const char *bytes, qsizetype len)
{
    *this << static_cast<quint32>(len);
    if (len) {
        writeRawData(bytes, len);
    }
}

qint64 DataStream::readRawData(char *buffer, qint64 len)
{
    checkDevice();

    return mDev->read(buffer, len);
}

DataStream &DataStream::operator<<(const QString &str)
{
    if (str.isNull()) {
        *this << (quint32)0xffffffff;
    } else {
        writeBytes(reinterpret_cast<const char *>(str.unicode()), sizeof(QChar) * str.length());
    }
    return *this;
}

DataStream &DataStream::operator>>(QString &str)
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

DataStream &DataStream::operator<<(const QByteArray &data)
{
    if (data.isNull()) {
        *this << (quint32)0xffffffff;
    } else {
        writeBytes(data.constData(), data.size());
    }
    return *this;
}

DataStream &DataStream::operator>>(QByteArray &data)
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

DataStream &DataStream::operator<<(const QDateTime &dt)
{
    *this << dt.date().toJulianDay() << dt.time().msecsSinceStartOfDay() << dt.timeSpec();
    if (dt.timeSpec() == Qt::OffsetFromUTC) {
        *this << dt.offsetFromUtc();
    } else if (dt.timeSpec() == Qt::TimeZone) {
        *this << dt.timeZone().id();
    }
    return *this;
}

DataStream &DataStream::operator>>(QDateTime &dt)
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
