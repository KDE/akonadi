/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include "dbusthread.h"

class DBusThreadManager : public QObject
{
  public:
    static DBusThreadManager *self();

  protected:
    virtual void customEvent( QEvent *event );

  private:
    DBusThreadManager();

    static DBusThreadManager* mSelf;
    static QMutex mMutex;
};

class DBusThreadEvent : public QEvent
{
  public:
    DBusThreadEvent( DBusThread *thread, const QString &service, const QString &path,
                     const QString &inteface, const QString &method,
                     const QList<QVariant> &arguments )
      : QEvent( QEvent::User ),
        mThread( thread ), mService( service ), mPath( path ), mInterface( inteface ),
        mMethod( method ), mArguments( arguments )
    {
    }

    DBusThread *mThread;
    QString mService;
    QString mPath;
    QString mInterface;
    QString mMethod;
    QList<QVariant> mArguments;
};

DBusThread::DBusThread( QObject *parent )
  : QThread( parent )
{
}

QList<QVariant> DBusThread::callDBus( const QString &service, const QString &path,
                                      const QString &interface, const QString &method,
                                      const QList<QVariant> &arguments )
{
  mData.clear();

  DBusThreadEvent *event = new DBusThreadEvent( this, service, path, interface,
                                                method, arguments );

  DBusThreadManager *manager = DBusThreadManager::self();

  mMutex.lock();

  QCoreApplication::instance()->postEvent( manager, event );

  mCondition.wait( &mMutex );
  mMutex.unlock();

  return mData;
}

void DBusThread::wakeup( const QList<QVariant> &result )
{
  mData = result;

  mMutex.lock();
  mCondition.wakeAll();
  mMutex.unlock();
}

DBusThreadManager* DBusThreadManager::mSelf = 0;
QMutex DBusThreadManager::mMutex;

DBusThreadManager::DBusThreadManager()
  : QObject( 0 )
{
}

DBusThreadManager *DBusThreadManager::self()
{
  mMutex.lock();

  if ( !mSelf ) {
      mSelf = new DBusThreadManager();

      /**
       * Move the proxy to the main (gui) thread, only this thread
       * can handle dbus calls.
       */
      mSelf->moveToThread( QCoreApplication::instance()->thread() );
  }

  mMutex.unlock();

  return mSelf;
}

void DBusThreadManager::customEvent( QEvent *_event )
{
  DBusThreadEvent *event = static_cast<DBusThreadEvent*>( _event );

  QDBusMessage message = QDBusMessage::createMethodCall( event->mService,
                                                         event->mPath,
                                                         event->mInterface,
                                                         event->mMethod );
  message.setArguments( event->mArguments );

  new DBusThreadProxy( event->mThread, message );
}


DBusThreadProxy::DBusThreadProxy( DBusThread *thread, const QDBusMessage &message )
  : mThread( thread )
{
  if ( !QDBusConnection::sessionBus().callWithCallback( message, this, SLOT( dbusReply( const QDBusMessage& ) ) ) )
    emit dbusReply( message.createError( QLatin1String("Call Error"), QLatin1String("callWithCallback() failed in DBusThreadProxy") ) );
}

void DBusThreadProxy::dbusReply( const QDBusMessage &message )
{
  if ( mThread )
    mThread->wakeup( message.arguments() );
  else
    qDebug( "DBusThreadProxy: thread was deleted during dbus call!" );

  deleteLater();
}

#include "dbusthread.moc"

