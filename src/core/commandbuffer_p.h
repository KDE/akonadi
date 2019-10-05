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

#ifndef AKONADICORE_COMMANDBUFFER_P_H_
#define AKONADICORE_COMMANDBUFFER_P_H_

class QObject;

#include <QMutex>
#include <QQueue>

#include <private/protocol_p.h>

#include "akonadicore_debug.h"

namespace Akonadi {

class CommandBufferLocker;
class CommandBufferNotifyBlocker;

class CommandBuffer
{
    friend class CommandBufferLocker;
    friend class CommandBufferNotifyBlocker;
public:
    struct Command {
        qint64 tag;
        Protocol::CommandPtr command;
    };

    CommandBuffer(QObject *parent, const char *notifySlot)
        : mParent(parent)
        , mNotifySlot(notifySlot)
    {
    }

    void enqueue(qint64 tag, const Protocol::CommandPtr &command)
    {
        mCommands.enqueue({ tag, command });
        if (mNotify) {
            const bool ok = QMetaObject::invokeMethod(mParent, mNotifySlot.constData(), Qt::QueuedConnection);
            Q_ASSERT(ok); Q_UNUSED(ok);
        }
    }

    inline Command dequeue()
    {
        return mCommands.dequeue();
    }

    inline bool isEmpty() const
    {
        return mCommands.isEmpty();
    }

    inline int size() const
    {
        return mCommands.size();
    }

private:
    QObject *mParent = nullptr;
    QByteArray mNotifySlot;

    QQueue<Command> mCommands;
    QMutex mLock;

    bool mNotify = true;
};

class CommandBufferLocker
{
public:
    explicit CommandBufferLocker(CommandBuffer *buffer)
        : mBuffer(buffer)
    {
        relock();
    }

    ~CommandBufferLocker()
    {
        unlock();
    }

    inline void unlock()
    {
        if (mLocked) {
            mBuffer->mLock.unlock();
            mLocked = false;
        }
    }

    inline void relock()
    {
        if (!mLocked) {
            mBuffer->mLock.lock();
            mLocked = true;
        }
    }

private:
    CommandBuffer *mBuffer = nullptr;
    bool mLocked = false;
};

class CommandBufferNotifyBlocker
{
public:
    explicit CommandBufferNotifyBlocker(CommandBuffer *buffer)
        : mBuffer(buffer)
    {
        mBuffer->mNotify = false;
    }

    ~CommandBufferNotifyBlocker()
    {
        unblock();
    }

    void unblock()
    {
        mBuffer->mNotify = true;
    }
private:
    CommandBuffer *mBuffer;
};

} // namespace

#endif
