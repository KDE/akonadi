/*
 * SPDX-FileCopyrightText: 2025 Andreas Hartmetz <ahartmetz@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QLocalSocket>

namespace Akonadi::Server
{

// This class only serves to implement a workaround for a QLocalSocket bug on Unix (which is due
// to a bug in QAbstractSocket that it uses internally). waitForBytesWritten() polls the socket
// descriptor for both read and write events; it can emit signals, among them readyRead(), handlers
// of which (API client code) can cause the write buffer to be flushed, so a later attempt to write
// more in waitForBytesWritten() itself then fails.
// But data was actually written in that case, so waitForBytesWritten() should return true.

// TODO: This code should be updated as a fix hopefully lands in Qt:
// - add proper Qt version check
// - remove this workaround when the minimum supported Qt version doesn't contain the bug anymore

// Note that the logic "not Windows -> Unix" matches the logic in qlocalsocket_p.h.
// Platforms that use TCP for local sockets seem to be rather exotic, but they also need the
// workaround because of the internal use of QAbstractSocket, and they are (usually) not Windows...
#if !defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 11, 0)
#define AKONADI_LOCALSOCKET_WORKAROUND
#endif

class LocalSocket : public QLocalSocket
{
    Q_OBJECT

public:
    explicit LocalSocket(QObject *parent);
#ifdef AKONADI_LOCALSOCKET_WORKAROUND
    bool waitForBytesWritten(int msecs = 30000) override;

private:
    quint32 mWriteCount = 0;
#endif
};

}
