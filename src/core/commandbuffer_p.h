/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICORE_COMMANDBUFFER_P_H_
#define AKONADICORE_COMMANDBUFFER_P_H_

class QObject;

#include <QMutex>
#include <QQueue>

#include <private/protocol_p.h>

#include "akonadicore_debug.h"

namespace Akonadi
{
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
        mCommands.enqueue({tag, command});
        if (mNotify) {
            const bool ok = QMetaObject::invokeMethod(mParent, mNotifySlot.constData(), Qt::QueuedConnection);
            Q_ASSERT(ok);
            Q_UNUSED(ok)
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
    Q_DISABLE_COPY_MOVE(CommandBuffer)

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
    Q_DISABLE_COPY_MOVE(CommandBufferLocker)

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
    Q_DISABLE_COPY_MOVE(CommandBufferNotifyBlocker)

    CommandBuffer *mBuffer;
};

} // namespace

#endif
