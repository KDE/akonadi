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

#include <kdebug.h>
#include <kglobal.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

using namespace Akonadi;

/**
 * @internal
 */
class Control::Private
{
  public:
    Private( Control *parent )
      : mParent( parent ), mEventLoop( 0 ), mSuccess( false ),
        mStarting( false ), mStopping( false )
    {
    }

    bool exec();
    void serverStarted();
    void serverStopped();

    Control *mParent;
    QEventLoop *mEventLoop;
    bool mSuccess;

    bool mStarting;
    bool mStopping;
};

class StaticControl : public Control
{
  public:
    StaticControl() : Control() {}
};

K_GLOBAL_STATIC( StaticControl, s_instance )


bool Control::Private::exec()
{
  mEventLoop = new QEventLoop( mParent );
  // safety timeout
  QTimer::singleShot( 10000, mEventLoop, SLOT(quit()) );
  mEventLoop->exec();
  mEventLoop->deleteLater();
  mEventLoop = 0;
  mStarting = false;
  mStopping = false;

  if ( !mSuccess )
    kWarning( 5250 ) << "Could not start/stop Akonadi!";
  return mSuccess;
}

void Control::Private::serverStarted()
{
  if ( mEventLoop && mEventLoop->isRunning() && mStarting ) {
    mEventLoop->quit();
    mSuccess = true;
  }
}

void Control::Private::serverStopped()
{
  if ( mEventLoop && mEventLoop->isRunning() && mStopping ) {
    mEventLoop->quit();
    mSuccess = true;
  }
}


Control::Control()
  : d( new Private( this ) )
{
  connect( ServerManager::instance(), SIGNAL(started()), SLOT(serverStarted()) );
  connect( ServerManager::instance(), SIGNAL(stopped()), SLOT(serverStopped()) );
}

Control::~Control()
{
  delete d;
}

bool Control::start()
{
  if ( s_instance->d->mStopping )
    return false;
  if ( ServerManager::isRunning() || s_instance->d->mEventLoop )
    return true;
  s_instance->d->mStarting = true;
  if ( !ServerManager::start() )
    return false;
  return s_instance->d->exec();
}

bool Control::stop()
{
  if ( s_instance->d->mStarting )
    return false;
  if ( !ServerManager::isRunning() || s_instance->d->mEventLoop )
    return true;
  s_instance->d->mStopping = true;
  if ( !ServerManager::stop() )
    return false;
  return s_instance->d->exec();
}

bool Control::restart()
{
  stop();
  start();
}

#include "control.moc"
