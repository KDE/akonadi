/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_NEPOMUKSEARCH_H
#define AKONADI_NEPOMUKSEARCH_H

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

#include "search/queryserviceclient.h"

namespace Akonadi {

class NepomukSearch : public QObject
{
  Q_OBJECT

  public:
    NepomukSearch( QObject* parent = 0 );
    ~NepomukSearch();

    QStringList search( const QString &query );

  private Q_SLOTS:
    void hitsAdded( const QList<Nepomuk::Search::Result>& entries );

  private:
    QSet<QString> mMatchingUIDs;
    Nepomuk::Search::QueryServiceClient* mSearchService;
};

}

#endif
