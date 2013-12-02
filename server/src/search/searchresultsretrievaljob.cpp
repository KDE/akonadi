/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "searchresultsretrievaljob.h"
#include "searchinstance.h"
#include "searchresultsretriever.h"

#include <agentsearchinterface.h>

using namespace Akonadi;

SearchResultsRetrievalJob::SearchResultsRetrievalJob( Akonadi::SearchRequest *request,
                                                      QObject* parent)
  : QObject(parent)
  , mRequest( request )
  , mActive( false )
  , mInstance( 0 )
{
}

SearchResultsRetrievalJob::~SearchResultsRetrievalJob()
{
}

void SearchResultsRetrievalJob::start( SearchInstance* instance )
{
  Q_ASSERT( mRequest );

  if ( mInstance ) {
    mActive = true;
    mInstance->search( mRequest->id, mRequest->query, mRequest->collectionId );
  } else {
    Q_EMIT requestCompleted( mRequest, QSet<qint64>() );
    deleteLater();
  }
}

void SearchResultsRetrievalJob::setResult( const QSet<qint64> &result )
{
  mResult = result;
  mActive = false;

  Q_EMIT requestCompleted( mRequest, result );
  deleteLater();
}
