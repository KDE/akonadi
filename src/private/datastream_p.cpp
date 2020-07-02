/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "datastream_p_p.h"

#ifdef Q_OS_WIN
#include <QEventLoop>
#include <QTimer>
#include <QLocalSocket>
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

void DataStream::waitForData(QIODevice *device, int timeoutMs)
{
#ifdef Q_OS_WIN
    // Apparently readyRead() gets emitted sometimes even if there are no data
    // so we will re-enter the wait again immediately
    while (device->bytesAvailable() == 0) {
        auto ls = qobject_cast<QLocalSocket*>(device);
        if (ls && ls->state() != QLocalSocket::ConnectedState) {
            throw ProtocolException("Socket not connected to server");
        }

        QEventLoop loop;
        QObject::connect(device, &QIODevice::readyRead, &loop, &QEventLoop::quit);
        if (ls) {
            QObject::connect(ls, &QLocalSocket::stateChanged, &loop, &QEventLoop::quit);
        }
        bool timeout = false;
        if (timeoutMs > 0) {
            QTimer::singleShot(timeoutMs, &loop, [&]() {
                timeout = true;
                loop.quit();
            });
        }
        loop.exec();
        if (timeout) {
            throw ProtocolException("Timeout while waiting for data");
        }
        if (ls && ls->state() != QLocalSocket::ConnectedState) {
            throw ProtocolException("Socket not connected to server");
        }
    }
#else
    if (!device->waitForReadyRead(timeoutMs)) {
        throw ProtocolException("Timeout while waiting for data");
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
        waitForData(mDev, mWaitTimeout.count());
    }
}

void DataStream::writeRawData(const char *data, int len)
{
    checkDevice();

    mWriteBuffer += QByteArray::fromRawData(data, len);
}

void DataStream::writeBytes(const char *bytes, int len)
{
    *this << static_cast<quint32> (len);
    if (len) {
        writeRawData(bytes, len);
    }
}

int DataStream::readRawData(char *buffer, int len)
{
    checkDevice();

    return mDev->read(buffer, len);
}
