/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "channel.h"
#include "akonadiprivate_debug.h"

#include <mutex>

using namespace Akonadi;

ChannelBase::ChannelBase(const QByteArray &id)
    : mId(id)
{

}

void ChannelBase::setCapacity(std::size_t capacity)
{
    if (!mQueueData) {
        qCWarning(AKONADIPRIVATE_LOG, "Changing capacity of a closed channel!");
        return;
    }

    std::unique_lock<boost::interprocess::interprocess_mutex> lock(mQueueData->mutex);
    mQueue->capacity = capacity;
}

std::size_t Akonadi::ChannelBase::capacity() const
{
    if (!mQueueData) {
        return 0;
    }

    std::unique_lock<boost::interprocess::interprocess_mutex> lock(mQueueData->mutex);
    return mQueueData->capacity;
}

bool ChannelBase::create()
{
    Q_ASSERT_X(mQueueData == nullptr, "ChannelBase", "Channel already opened in 'attach' mode");
    if (mQueueData) {
        return false;
    }

    try {
        mShm = {boost::interprocess::create_only, mId.constData(), boost::interprocess::read_write};
        mShm.truncate(sizeof(Queue));

        mMapped = {mShm, boost::interprocess::read_write};

        mQueueData = new (mMapped.get_address()) Queue;
    } catch (const boost::interprocess::interprocess_exception &e) {
        qCCritical(AKONADIPRIVATE_LOG, "Failed to create Channel shared memory: %s", e.what());
        mQueueData = nullptr;
        mMapped = {};
        mShm.remove(mId.constData());
        mShm = {};
        return false;
    }

    return true;
}

bool ChannelBase::attach()
{
    Q_ASSERT_X(mQueueData == nullptr, "ChannelBase", "Channel already opened in 'create' mode");
    if (mQueueData) {
        return false;
    }

    try {
        mShm = {boost::interprocess::open_only, mId.constData(), boost::interprocess::read_write};
        mMapped = {mShm, boost::interprocess::read_write};
        mQueueData = static_cast<Queue*>(mMapped.get_address());
    } catch (const boost::interprocess::interprocess_exception &e) {
        qCCritical(AKONADIPRIVATE_LOG, "Failed to attach to Channel shared memory: %s", e.what());
        mQueueData = nullptr;
        mMapped = {};
        mShm = {};
        return false;
    }

    return true;
}

