/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentsearchinterface.h"
#include "collection.h"

#include <QObject>

class KJob;

namespace Akonadi
{
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
    AgentSearchInterface *const q;
};

}

