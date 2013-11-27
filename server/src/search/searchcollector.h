/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_SEARCHCOLLECTOR_H
#define AKONADI_SEARCHCOLLECTOR_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <QDBusConnection>

#include <akdbus.h>
#include <akdebug.h>

class OrgFreedesktopAkonadiAgentSearchInterface;
class OrgFreedesktopAkonadiResourceInterface;

namespace Akonadi {

class SearchJob;

struct SearchTask {
  QByteArray id;
  QString query;
  QMultiMap<QByteArray /* resource name */, qint64 /* collection id */> collections;
  QStringList mimeTypes;
};

class SearchCollector: public QObject
{
    Q_OBJECT

  public:
    static SearchCollector *self();

    QVector<qint64> exec( SearchTask *task, bool *ok );

  private Q_SLOTS:
    void serviceOwnerChanged( const QString &serviceName, const QString &oldOwner, const QString &newOwner );
    void searchJobFinished( SearchJob *job );

  private:
    SearchCollector();
    ~SearchCollector();

    template <typename T>
    T *findInterface( const QByteArray &resourceName, QHash<QString, T> &cache )
    {
      T *iface = cache.value( resourceName );
      if ( iface && iface.isValid() ) {
        return iface;
      }

      delete iface;
      iface = new T( AkDBus::agentServiceName( resourceName, AkDBus::Agent ),
                      QLatin1String( "/" ), mDBusConnection, this );
      if ( !iface || !iface->isValid() ) {
        // Not an error - agent simply does not have needed the interface
        delete iface;
        return 0;
      }
      #if QT_VERSION >= 0x040800
      // DBus calls can take some time to reply -- e.g. if a huge local mbox has to be parsed first.
      iface->setTimeout( 5 * 60 * 1000 ); // 5 minutes, rather than 25 seconds
      #endif
      cache.insert( resourceName, iface );
      return iface;
    }

    static SearchCollector *m_instance;
    QWaitCondition mWait;
    QReadWriteLock mLock;
    QDBusConnection mDBusConnection;
    QHash<QString, OrgFreedesktopAkonadiAgentSearchInterface> mSearchInterfaces;
    QHash<QString, OrgFreedesktopAkonadiResourceInterface> mResourceInterfaces;
};

}

#endif // AKONADI_SEARCHCOLLECTOR_H
