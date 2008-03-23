/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_NEPOMUKMANAGER_H
#define AKONADI_NEPOMUKMANAGER_H

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QObject>

#include "abstractsearchmanager.h"

#include "searchinterface.h"
#include "searchqueryinterface.h"

namespace Akonadi {

class NepomukManager : public QObject, public AbstractSearchManager
{
  Q_OBJECT

  public:
    NepomukManager( QObject* parent = 0 );
    ~NepomukManager();

    bool addSearch( const Location &location );
    bool removeSearch( qint64 location );

  private:
    void reloadSearches();
    void stopSearches();

  private Q_SLOTS:
    void hitsAdded( const QStringList &hits );
    void hitsRemoved( const QStringList &hits );

  private:
    bool mValid;
    QMutex mMutex;

    QHash<org::kde::Akonadi::SearchQuery*, qint64> mQueryMap;
    QHash<qint64, org::kde::Akonadi::SearchQuery*> mQueryInvMap;

    org::kde::Akonadi::Search *mSearchInterface;
};

}

#endif
