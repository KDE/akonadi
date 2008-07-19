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

#include <kdebug.h>
#include <kglobal.h>

#include <QtCore/QEventLoop>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.freedesktop.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.freedesktop.Akonadi")

using namespace Akonadi;

/**
 * @internal
 */
class Control::Private
{
  public:
    Private( Control *parent )
      : mParent( parent ), mEventLoop( 0 ), mSuccess( false )
    {
    }

    bool startInternal();
    void serviceOwnerChanged( const QString&, const QString&, const QString& );

    Control *mParent;
    QEventLoop *mEventLoop;
    bool mSuccess;
};

class StaticControl : public Control
{
  public:
    StaticControl() : Control() {}
};

K_GLOBAL_STATIC( StaticControl, s_instance )


bool Control::Private::startInternal()
{
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE ) || mEventLoop )
    return true;

  QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService( AKONADI_CONTROL_SERVICE );
  if ( !reply.isValid() ) {
    kWarning( 5250 ) << "Unable to start Akonadi control process: "
                     << reply.error().message();

    // start via D-Bus .service file didn't work, let's try starting the process manually
    if ( reply.error().type() == QDBusError::ServiceUnknown ) {
      const bool ok = QProcess::startDetached( QLatin1String("akonadi_control") );
      if ( !ok ) {
        kWarning( 5250 ) << "Error: unable to execute binary akonadi_control";
        return false;
      }
    } else {
      return false;
    }
  }

  mEventLoop = new QEventLoop( mParent );
  // safety timeout
  QTimer::singleShot( 10000, mEventLoop, SLOT(quit()) );
  mEventLoop->exec();
  mEventLoop->deleteLater();
  mEventLoop = 0;

  if ( !mSuccess )
    kWarning( 5250 ) << "Could not start Akonadi!";
  return mSuccess;
}

void Control::Private::serviceOwnerChanged( const QString & name, const QString & oldOwner, const QString & newOwner )
{
  Q_UNUSED( oldOwner );
  if ( name == AKONADI_SERVER_SERVICE && !newOwner.isEmpty() && mEventLoop && mEventLoop->isRunning() ) {
    mEventLoop->quit();
    mSuccess = true;
  }
}


Control::Control()
  : d( new Private( this ) )
{
  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( QString, QString, QString ) ),
           SLOT( serviceOwnerChanged( QString, QString, QString ) ) );
}

Control::~Control()
{
  delete d;
}

bool Control::start()
{
  return s_instance->d->startInternal();
}

#include "control.moc"
