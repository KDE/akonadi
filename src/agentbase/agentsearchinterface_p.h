/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKOANDI_AGENTSEARCHINTERFACE_P_H
#define AKOANDI_AGENTSEARCHINTERFACE_P_H

#include "agentsearchinterface.h"
#include "collection.h"

#include <QObject>


class KJob;

namespace Akonadi {


class AgentSearchInterfacePrivate : public QObject
{
    Q_OBJECT
public:
    explicit AgentSearchInterfacePrivate(AgentSearchInterface *qq);

    void search(const QByteArray &searchId, const QString &query, quint64 collectionId);
    void addSearch(const QString &query, const QString &queryLanguage, quint64 resultCollectionId);
    void removeSearch(quint64 resultCollectionId);

    QByteArray mSearchId;
    qint64 mCollectionId;

private Q_SLOTS:
    void delayedInit();
    void collectionReceived(KJob *job);

private:
    AgentSearchInterface *q;
};

}

#endif
