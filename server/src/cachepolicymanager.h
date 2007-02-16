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

#ifndef AKONADI_CACHEPOLICYMANAGER_H
#define AKONADI_CACHEPOLICYMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace Akonadi {

/**
  D-Bus interface for accessing and manipulating cache policies.
*/
class CachePolicyManager : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.CachePolicyManager" )

  public:
    /**
      Constructs a new CachePolicyManager.
      @param parent The parent object.
    */
    CachePolicyManager( QObject *parent = 0 );

  public slots:
    /**
      List all cache policies.
      @todo Return a list of structures instead of a string representation
    */
    Q_SCRIPTABLE QStringList cachePolicies();

    /**
      Deletes the specified cache policy.
      @param id The cache policy id.
      @return ture if deletion has been successful.
    */
    Q_SCRIPTABLE bool deleteCachePolicy( int id );

    /**
      Adds a new cache policy.
      @param name A unique name for this policy.
      @param expireTime Time in minutes to keep items, -1 means no expire.
      @param offlineParts Item parts to download for offline useage.
      @returns The id of the new policy, -1 on failure
    */
    Q_SCRIPTABLE int addCachePolicy( const QString &name, int expireTime, const QString &offlineParts );
};

}

#endif
