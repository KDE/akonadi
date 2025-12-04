/*
 * SPDX-FileCopyrightText: 2025 Andreas Hartmetz <ahartmetz@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "localsocket_p.h"

#include <QLibraryInfo>
#include <QVersionNumber>

using namespace Akonadi::Server;

LocalSocket::LocalSocket(QObject *parent)
    : QLocalSocket(parent)
{
#ifdef AKONADI_LOCALSOCKET_WORKAROUND
    if (QLibraryInfo::version() >= QVersionNumber(6, 10, 2)) {
        return;
    }
    // channelBytesWritten() would be better because it can also be emitted recursively, but
    // bytesWritten() is probably good enough for this workaround. The "channel signals" of
    // QLocalSocket (at least on Unix) are currently not emitted due to another bug.
    connect(this, &QIODevice::bytesWritten, this, [this]() {
        mWriteCount++;
    });
}

bool LocalSocket::waitForBytesWritten(int msecs)
{
    const quint32 writeCountBefore = mWriteCount;
    const bool ok = QLocalSocket::waitForBytesWritten(msecs);
    return ok || mWriteCount != writeCountBefore; // cppcheck-suppress constStatement
#endif
}
