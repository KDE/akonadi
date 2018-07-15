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

#ifndef AKONADIPRIVATE_CHANNEL_P_H_
#define AKONADIPRIVATE_CHANNEL_P_H_

#include "akonadiprivate_export.h"

#include <QByteArray>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include <deque>
#include <mutex>

namespace Akonadi {

class ChannelBase
{
public:
    explicit ChannelBase(const QByteArray &id);
    ChannelBase(const ChannelBase &other) = delete;
    ~ChannelBase() = default;

    void setCapacity(std::size_t capacity);
    Q_REQUIRED_RESULT std::size_t capacity() const;

    ChannelBase &operator=(const ChannelBase &other) = delete;

    /// Creates a new channel
    bool create();
    /// Attaches to an existing channel with given id
    bool attach();

protected:
    struct Queue {
        boost::interprocess::interprocess_condition full;
        boost::interprocess::interprocess_condition empty;
        boost::interprocess::interprocess_mutex mutex;
        std::size_t capacity;
    };

    const QByteArray mId;
    boost::interprocess::shared_memory_object mShm;
    boost::interprocess::mapped_region mMapped;
    Queue *mQueueData = nullptr;
}

template<typename T>
class Channel : public ChannelBase
{
public:
    explicit Channel(const QByteArray &id) = default;
    ~Channel() = default;

    void enqueue(T &&entity)
    {
        Q_ASSERT(mQueueData);
        std::unique_lock<boost::interprocess::interprocess_mutex> lock(mQueueData->mutex);
        while (mQueue.size() >= mQueueData->capacity) {
            mQueueData->full.wait(lock);
        }
        mQueue.emplace_back(entity);
        mQueueData->empty.wake_one();
    }

    Q_REQUIRED_RESULT T &&dequeue()
    {
        Q_ASSERT(mQueueData);
        std::unique_lock<boost::interprocess::interprocess_mutex> lock(mQueueData->mutex);
        while (mQueue.empty()) {
            mQueueData->empty.wait(lock);
        }
        T elem = std::move(mQueue.front());
        mQueue.pop();
        mQueueData->full.wake_one();
        return elem; // moves
    }

private:
    std::deque<T> mQueue;
};

} // namespace Akonadi

#endif
