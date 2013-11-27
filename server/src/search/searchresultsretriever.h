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

#include "scope.h"

namespace Akonadi
{

class AkonadiConnection;

class SearchResultsRetriever
{
  public:
    SearchResultsRetriever( AkonadiConnection *connection );
    ~SearchResultsRetriever();

    void setQuery( const QString &query );
    void setCollections( const Scope &collections );
    void setMimeTypes( const QStringList &mimeTypes );

    QVector<qint64> exec( bool *ok = 0 );

  private:
    AkonadiConnection *mConnection;
    QString mQuery;
    Scope mCollections;
    QStringList mMimeTypes;

    QVector<qint64> mResults;
};

}

#endif // AKONADI_SEARCHRESULTSRETRIEVER_H
