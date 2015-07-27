/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "control.h"
#include "servermanager.h"

#include <qdebug.h>

#include <QtCore/QEventLoop>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QPointer>

using namespace Akonadi;

namespace Akonadi
{
namespace Internal
{

class StaticControl : public Control
{
public:
    StaticControl()
        : Control()
    {
    }
};

}

Q_GLOBAL_STATIC(Internal::StaticControl, s_instance)

/**
 * @internal
 */
class Control::Private
{
public:
    Private(Control *parent)
        : mParent(parent)
        , mEventLoop(0)
        , mSuccess(false)
        , mStarting(false)
        , mStopping(false)
    {
    }

    ~Private()
    {
    }

    void cleanup()
    {
    }

    bool exec();
    void serverStateChanged(ServerManager::State state);

    QPointer<Control> mParent;
    QEventLoop *mEventLoop;
    bool mSuccess;

    bool mStarting;
    bool mStopping;
};

bool Control::Private::exec()
{
    qDebug() << "Starting/Stopping Akonadi (using an event loop).";
    mEventLoop = new QEventLoop(mParent);
    mEventLoop->exec();
    mEventLoop->deleteLater();
    mEventLoop = 0;

    if (!mSuccess) {
        qWarning() << "Could not start/stop Akonadi!";
    }

    mStarting = false;
    mStopping = false;

    const bool rv = mSuccess;
    mSuccess = false;
    return rv;
}

void Control::Private::serverStateChanged(ServerManager::State state)
{
    qDebug() << state;
    if (mEventLoop && mEventLoop->isRunning()) {
        // ignore transient states going into the right direction
        if ((mStarting && (state == ServerManager::Starting || state == ServerManager::Upgrading)) ||
                (mStopping && state == ServerManager::Stopping)) {
            return;
        }
        mEventLoop->quit();
        mSuccess = (mStarting && state == ServerManager::Running) || (mStopping && state == ServerManager::NotRunning);
    }
}

Control::Control()
    : d(new Private(this))
{
    connect(ServerManager::self(), SIGNAL(stateChanged(Akonadi::ServerManager::State)),
            SLOT(serverStateChanged(Akonadi::ServerManager::State)));
    // mProgressIndicator is a widget, so it better be deleted before the QApplication is deleted
    // Otherwise we get a crash in QCursor code with Qt-4.5
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
    }
}

Control::~Control()
{
    delete d;
}

bool Control::start()
{
    if (ServerManager::state() == ServerManager::Stopping) {
        qDebug() << "Server is currently being stopped, wont try to start it now";
        return false;
    }
    if (ServerManager::isRunning() || s_instance->d->mEventLoop) {
        qDebug() << "Server is already running";
        return true;
    }
    s_instance->d->mStarting = true;
    if (!ServerManager::start()) {
        qDebug() << "ServerManager::start failed -> return false";
        return false;
    }
    return s_instance->d->exec();
}

bool Control::stop()
{
    if (ServerManager::state() == ServerManager::Starting) {
        return false;
    }
    if (!ServerManager::isRunning() || s_instance->d->mEventLoop) {
        return true;
    }
    s_instance->d->mStopping = true;
    if (!ServerManager::stop()) {
        return false;
    }
    return s_instance->d->exec();
}

bool Control::restart()
{
    if (ServerManager::isRunning()) {
        if (!stop()) {
            return false;
        }
    }
    return start();
}

}

#include "moc_control.cpp"
