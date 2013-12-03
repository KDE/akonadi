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

#ifndef AKONADI_SEARCHRESULTSRETRIEVER_H
#define AKONADI_SEARCHRESULTSRETRIEVER_H

#include <QVector>
#include <QStringList>
#include <QMultiMap>

#include "exception.h"
#include "scope.h"

namespace Akonadi
{

class AkonadiConnection;

class SearchRequest
{
  public:
    SearchRequest()
    : processed( false )
    {
    }

    QByteArray id;
    QString query;
    QString resourceId;
    qint64 collectionId;
    QStringList mimeTypes;
    QString errorMsg;
    uint processed : 1;

    QSet<qint64> result;
};

class SearchResultsRetriever
{
  public:
    SearchResultsRetriever( AkonadiConnection *connection );
    ~SearchResultsRetriever();

    void setQuery( const QString &query );
    void setCollections( const QVector<qint64> &collections );
    void setMimeTypes( const QStringList &mimeTypes );

    QSet<qint64> exec( bool *ok = 0 );

  private:
    AkonadiConnection *mConnection;
    QString mQuery;
    QVector<qint64> mCollections;
    QStringList mMimeTypes;
};

AKONADI_EXCEPTION_MAKE_INSTANCE( SearchResultsRetrieverException );

}

#endif // AKONADI_SEARCHRESULTSRETRIEVER_H
