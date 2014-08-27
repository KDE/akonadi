/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_NEPOMUKSEARCHENGINE_H
#define AKONADI_NEPOMUKSEARCHENGINE_H

#include "abstractsearchengine.h"

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QObject>

#include <src/nepomuk/queryserviceclient.h>

namespace Akonadi {
namespace Server {

class NotificationCollector;

class NepomukSearchEngine : public QObject, public AbstractSearchEngine
{
    Q_OBJECT

public:
    NepomukSearchEngine(QObject *parent = 0);
    ~NepomukSearchEngine();

    void addSearch(const Collection &collection);
    void removeSearch(qint64 collection);

private:
    void stopSearches();

private Q_SLOTS:
    void reloadSearches();
    void hitsAdded(const QList<Nepomuk::Query::Result> &entries);
    void hitsRemoved(const QList<Nepomuk::Query::Result> &entries);
    void finishedListing();

private:
    QMutex mMutex;
    NotificationCollector *mCollector;

    QHash<Nepomuk::Query::QueryServiceClient *, qint64> mQueryMap;
    QHash<qint64, Nepomuk::Query::QueryServiceClient *> mQueryInvMap;

    bool m_liveSearchEnabled;
};

} // namespace Server
} // namespace Akonadi

#endif
