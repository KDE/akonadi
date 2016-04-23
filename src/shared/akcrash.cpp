/*
 * This file is part of the KDE Libraries
 * Copyright (C) 2000 Timo Hummel <timo.hummel@sap.com>
 *                    Tom Braun <braunt@fh-konstanz.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "akcrash.h"

#include <QDebug>
#include <QtCore/QString>

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

QString akBacktrace()
{
    QString s;

    /* FIXME: is there an equivalent for darwin, *BSD, or windows? */
#ifdef HAVE_BACKTRACE
    void *trace[256];
    int n = backtrace(trace, 256);
    if (!n) {
        return s;
    }

    char **strings = backtrace_symbols(trace, n);

    s = QStringLiteral("[\n");

    for (int i = 0; i < n; ++i) {
        s += QString::number(i) +
             QLatin1String(": ") +
             QLatin1String(strings[i]) + QLatin1String("\n");
    }
    s += QLatin1String("]\n");

    if (strings) {
        free(strings);
    }
#endif

    return s;
}

static AkonadiCrash::HandlerType s_emergencyMethod = 0;
static AkonadiCrash::HandlerType s_shutdownMethod = 0;
static int recursionCount = 0;

void AkonadiCrash::setEmergencyMethod(HandlerType method)
{
    s_emergencyMethod = method;
}

void AkonadiCrash::setShutdownMethod(HandlerType method)
{
    s_shutdownMethod = method;
}

static void defaultCrashHandler(int sig)
{
    ++recursionCount;
    if (recursionCount <= 2) {
        if (sig != SIGTERM && sig != SIGINT) {
            if (recursionCount == 1) {
                qCritical("%s", qPrintable(akBacktrace()));
            } else { // fall back to something more simple in case the other one crashed itself
                fprintf(stderr, "%s", akBacktrace().toLatin1().data());
            }

            if (s_emergencyMethod) {
                s_emergencyMethod(sig);
            }
        } else {
            if (s_shutdownMethod) {
                s_shutdownMethod(sig);
            }
        }
    }
#ifdef Q_CC_MINGW
    _Exit(255);
#else
    _exit(255);
#endif
}

void AkonadiCrash::init()
{
    HandlerType handler = defaultCrashHandler;

#if defined Q_OS_UNIX && !defined ENABLE_ASAN
    if (!handler) {
        handler = SIG_DFL;
    }

    sigset_t mask;
    sigemptyset(&mask);

#ifdef SIGSEGV
    signal(SIGSEGV, handler);
    sigaddset(&mask, SIGSEGV);
#endif
#ifdef SIGFPE
    signal(SIGFPE, handler);
    sigaddset(&mask, SIGFPE);
#endif
#ifdef SIGILL
    signal(SIGILL, handler);
    sigaddset(&mask, SIGILL);
#endif
#ifdef SIGABRT
    signal(SIGABRT, handler);
    sigaddset(&mask, SIGABRT);
#endif
#ifdef SIGTERM
    signal(SIGTERM, handler);
    sigaddset(&mask, SIGTERM);
#endif
#ifdef SIGINT
    signal(SIGINT, handler);
    sigaddset(&mask, SIGINT);
#endif

    sigprocmask(SIG_UNBLOCK, &mask, 0);
#endif //Q_OS_UNIX

}
