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
    , mWaitTimeout(30000)
{
}

DataStream::DataStream(QIODevice *device)
    : mDev(device)
    , mWaitTimeout(30000)
{
}

DataStream::~DataStream()
{
}

void DataStream::waitForData(QIODevice *device, int timeoutMs)
{
#ifdef Q_OS_WIN
    // Apparently readyRead() gets emitted sometimes even if there are no data
    // so we will re-enter the wait again immediatelly
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

int DataStream::waitTimeout() const
{
    return mWaitTimeout;
}
void DataStream::setWaitTimeout(int timeout)
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

void DataStream::writeRawData(const char *data, int len)
{
    checkDevice();

    int ret = mDev->write(data, len);
    if (ret != len) {
        // TODO: Try to write data again unless ret is -1?
        throw ProtocolException("Failed to write all data");
    }
}

void DataStream::writeBytes(const char *bytes, int len)
{
    *this << (quint32) len;
    if (len) {
        writeRawData(bytes, len);
    }
}

int DataStream::readRawData(char *buffer, int len)
{
    checkDevice();

    return mDev->read(buffer, len);
}
