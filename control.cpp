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
#include "ui_controlprogressindicator.h"
#include "selftestdialog.h"
#include "erroroverlay.h"

#include <kdebug.h>
#include <kglobal.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include <QFrame>

using namespace Akonadi;

class ControlProgressIndicator : public QFrame
{
  public:
    ControlProgressIndicator( QWidget *parent = 0 ) :
      QFrame( parent )
    {
      setWindowModality( Qt::ApplicationModal );
      resize( 400, 100 );
      setWindowFlags( Qt::FramelessWindowHint | Qt::Dialog );
      ui.setupUi( this );

      setFrameShadow( QFrame::Plain );
      setFrameShape( QFrame::Box );
    }

    void setMessage( const QString &msg )
    {
      ui.statusLabel->setText( msg );
    }

    Ui::ControlProgressIndicator ui;
};

/**
 * @internal
 */
class Control::Private
{
  public:
    Private( Control *parent )
      : mParent( parent ), mEventLoop( 0 ),
        mProgressIndicator( 0 ),
        mSuccess( false ),
        mStarting( false ), mStopping( false )
    {
    }

    void setupProgressIndicator( const QString &msg, QWidget *parent = 0 )
    {
      if ( mProgressIndicator )
        return;
      mProgressIndicator = new ControlProgressIndicator( parent );
      mProgressIndicator->setMessage( msg );
    }

    void createErrorOverlays()
    {
      foreach ( QWidget* widget, mPendingOverlays )
        new ErrorOverlay( widget );
      mPendingOverlays.clear();
    }

    bool exec();
    void serverStarted();
    void serverStopped();

    Control *mParent;
    QEventLoop *mEventLoop;
    ControlProgressIndicator *mProgressIndicator;
    QList<QWidget*> mPendingOverlays;
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
  if ( mProgressIndicator )
    mProgressIndicator->show();
  mEventLoop = new QEventLoop( mParent );
  // safety timeout
  QTimer::singleShot( 10000, mEventLoop, SLOT(quit()) );
  mEventLoop->exec();
  mEventLoop->deleteLater();
  mEventLoop = 0;

  if ( !mSuccess ) {
    kWarning( 5250 ) << "Could not start/stop Akonadi!";
    if ( mProgressIndicator && mStarting ) {
      SelfTestDialog dlg( mProgressIndicator->parentWidget() );
      dlg.exec();
    }
  }

  delete mProgressIndicator;
  mProgressIndicator = 0;
  mStarting = false;
  mStopping = false;

  const bool rv = mSuccess;
  mSuccess = false;
  return rv;
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
  if ( ServerManager::isRunning() ) {
    if ( !stop() )
      return false;
  }
  return start();
}

bool Control::start(QWidget * parent)
{
  s_instance->d->setupProgressIndicator( i18n( "Starting Akonadi server..." ), parent );
  return start();
}

bool Control::stop(QWidget * parent)
{
  s_instance->d->setupProgressIndicator( i18n( "Stopping Akonadi server..." ), parent );
  return stop();
}

bool Control::restart(QWidget * parent)
{
  if ( ServerManager::isRunning() ) {
    if ( !stop( parent ) )
      return false;
  }
  return start( parent );
}

void Control::widgetNeedsAkonadi(QWidget * widget)
{
  s_instance->d->mPendingOverlays.append( widget );
  // delay the overlay creation since we rely on widget being reparented
  // correctly already
  QTimer::singleShot( 0, s_instance, SLOT(createErrorOverlays()) );
}

#include "control.moc"
