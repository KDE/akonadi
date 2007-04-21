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
#include <kstaticdeleter.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QEventLoop>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.kde.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.kde.Akonadi")

using namespace Akonadi;

static KStaticDeleter<Control> sControlDeleter;

class Control::Private
{
  public:
    Private( Control *parent )
      : mParent( parent ), mEventLoop( 0 )
    {
    }

    void startInternal();
    void serviceOwnerChanged( const QString&, const QString&, const QString& );
    static Control* self();

    Control *mParent;
    QEventLoop *mEventLoop;

    static Control* mInstance;
};

Control* Control::Private::mInstance = 0;

Control* Control::Private::self()
{
  if ( !mInstance )
    sControlDeleter.setObject( mInstance, new Control() );

  return mInstance;
}

void Control::Private::startInternal()
{
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE ) || mEventLoop )
    return;

  QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService( AKONADI_CONTROL_SERVICE );
  if ( !reply.isValid() ) {
    qWarning( "Unable to start Akonadi control process: %s", qPrintable( reply.error().message() ) );
    return;
  }

  mEventLoop = new QEventLoop( mParent );
  mEventLoop->exec();
}

void Control::Private::serviceOwnerChanged( const QString & name, const QString & oldOwner, const QString & newOwner )
{
  Q_UNUSED( oldOwner );
  if ( name == AKONADI_SERVER_SERVICE && !newOwner.isEmpty() && mEventLoop ) {
    mEventLoop->quit();
    delete mEventLoop;
    mEventLoop = 0;
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

void Control::start()
{
  Private::self()->d->startInternal();
}

#include "control.moc"
