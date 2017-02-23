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

using namespace Akonadi;
using namespace Akonadi::Protocol;

DataStream::DataStream()
    : mDev(Q_NULLPTR)
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
        if (!mDev->waitForReadyRead(mWaitTimeout)) {
            throw ProtocolException("Timeout while waiting for data");
        }
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
