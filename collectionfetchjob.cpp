/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collection.h"
#include "collectionfetchjob.h"
#include "messagecollection.h"

#include <QTimer>

using namespace PIM;

// ### just for testing, remove as soon as we have a real backend
#define DUMMY_FETCH_JOB
#ifdef DUMMY_FETCH_JOB
#include <QHash>
static Collection* col = 0;
QHash<DataReference, Collection*> global_collection_map;
#endif

PIM::CollectionFetchJob::CollectionFetchJob( const DataReference & ref )
{
#ifdef DUMMY_FETCH_JOB
  if ( global_collection_map.contains( ref ) ) {
    Collection *c = global_collection_map.value( ref );
    if ( dynamic_cast<MessageCollection*>( c ) )
      col = new MessageCollection( *static_cast<MessageCollection*>( c ) );
    else
      col = new Collection( *c );
  } else
    col = 0;
#else
  Q_UNUSED( ref );
#endif
}

PIM::CollectionFetchJob::~CollectionFetchJob()
{
}

Collection * PIM::CollectionFetchJob::collection() const
{
#ifdef DUMMY_FETCH_JOB
  return col;
#else
  return 0;
#endif
}

void PIM::CollectionFetchJob::emitDone()
{
  emit done( this );
}

void PIM::CollectionFetchJob::doStart()
{
  QTimer::singleShot( 0, this, SLOT( emitDone() ) );
}

#include "collectionfetchjob.moc"
