/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONMODEL_P_H
#define AKONADI_COLLECTIONMODEL_P_H

// @cond PRIVATE

#include "collection.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QStringList>

class KJob;

namespace Akonadi {


class CollectionModel;
class CollectionStatistics;
class Monitor;
class Session;

class CollectionModelPrivate
{
  public:
    Q_DECLARE_PUBLIC( CollectionModel )
    CollectionModelPrivate( CollectionModel *parent )
      : q_ptr( parent ), fetchStatistics( false ), unsubscribed( false )
    {
    }

    virtual ~CollectionModelPrivate() {}

    CollectionModel *q_ptr;
    QHash<Collection::Id, Collection> collections;
    QHash<Collection::Id, QList<Collection::Id> > childCollections;
    Monitor *monitor;
    Session *session;
    QStringList mimeTypes;
    bool fetchStatistics;
    bool unsubscribed;

    void init();
    void startFirstListJob();
    void collectionRemoved( const Akonadi::Collection& );
    void collectionChanged( const Akonadi::Collection& );
    void updateDone( KJob* );
    void collectionStatisticsChanged( Collection::Id, const Akonadi::CollectionStatistics& );
    void listDone( KJob* );
    void editDone( KJob* );
    void dropResult( KJob* );
    void collectionsChanged( const Akonadi::Collection::List &cols );

    void updateSupportedMimeTypes( Collection col )
    {
      QStringList l = col.contentMimeTypes();
      for ( QStringList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
        if ( (*it) == Collection::mimeType() )
          continue;
        if ( !mimeTypes.contains( *it  ) )
          mimeTypes << *it;
      }
    }
};

}

#endif
