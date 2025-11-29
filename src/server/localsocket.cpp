/*
 * SPDX-FileCopyrightText: 2025 Andreas Hartmetz <ahartmetz@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "localsocket_p.h"

using namespace Akonadi::Server;

LocalSocket::LocalSocket(QObject *parent)
    : QLocalSocket(parent)
{
#ifdef AKONADI_LOCALSOCKET_WORKAROUND
    connect(this, &QIODevice::channelBytesWritten, this, [this]() {
        // There should be just one channel in a network socket-type QIODevice anyway
        Q_ASSERT(currentReadChannel() == 0);
        mWriteCount++;
    });
}

bool LocalSocket::waitForBytesWritten(int msecs)
{
    const quint32 writeCountBefore = mWriteCount;
    const bool ok = QLocalSocket::waitForBytesWritten(msecs);
    return ok || mWriteCount != writeCountBefore;
#endif
}
