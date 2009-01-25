/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dao.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>

Akonadi::Collection::List DAO::showCollections() const
{
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( Akonadi::Collection::root(),
                                                                      Akonadi::CollectionFetchJob::Recursive );
  job->exec();

  return job->collections();
}

Akonadi::Collection DAO::getCollectionByName( const QString &collectionName ) const
{
  const Akonadi::Collection::List collections = showCollections();

  foreach ( const Akonadi::Collection &collection, collections ) {
    if ( collectionName == collection.name() )
      return collection;
  }

  return Akonadi::Collection();
}

bool DAO::insertItem( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection );

  return job->exec();
}
