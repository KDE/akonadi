/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KNUTRESOURCE_H
#define KNUTRESOURCE_H

#include <collection.h>
#include <item.h>
#include <resourcebase.h>

#include <agentsearchinterface.h>
#include <searchquery.h>
#include <xmldocument.h>

#include "settings.h"

class QFileSystemWatcher;

class KnutResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2, public Akonadi::AgentSearchInterface
{
    Q_OBJECT

public:
    using Akonadi::AgentBase::ObserverV2::collectionChanged; // So we don't trigger -Woverloaded-virtual
    explicit KnutResource(const QString &id);
    ~KnutResource() override;

public Q_SLOTS:
    void configure(WId windowId) override;

protected:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
#ifdef DO_IT_THE_OLD_WAY
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
#endif
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &ref) override;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

    void search(const QString &query, const Akonadi::Collection &collection) override;
    void addSearch(const QString &query, const QString &queryLanguage, const Akonadi::Collection &resultCollection) override;
    void removeSearch(const Akonadi::Collection &resultCollection) override;

private:
    static QSet<qint64> parseQuery(const QString &queryString);

private Q_SLOTS:
    void load();
    void save();

private:
    Akonadi::XmlDocument mDocument;
    QFileSystemWatcher *mWatcher = nullptr;
    KnutSettings *mSettings = nullptr;
};

#endif
