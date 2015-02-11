/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
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

#ifndef KNUTRESOURCE_H
#define KNUTRESOURCE_H

#include <resourcebase.h>
#include <collection.h>
#include <item.h>

#include <xmldocument.h>
#include <agentsearchinterface.h>
#include <searchquery.h>

#include "settings.h"

class QFileSystemWatcher;

class KnutResource : public Akonadi::ResourceBase,
                     public Akonadi::AgentBase::ObserverV2,
                     public Akonadi::AgentSearchInterface
{
    Q_OBJECT

public:
    KnutResource(const QString &id);
    ~KnutResource();

public Q_SLOTS:
    virtual void configure(WId windowId) Q_DECL_OVERRIDE;

protected:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection ) Q_DECL_OVERRIDE;
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts ) Q_DECL_OVERRIDE;
    void itemRemoved( const Akonadi::Item &ref ) Q_DECL_OVERRIDE;
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination ) Q_DECL_OVERRIDE;

    void search(const QString &query, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void addSearch(const QString &query, const QString &queryLanguage, const Akonadi::Collection &resultCollection) Q_DECL_OVERRIDE;
    void removeSearch(const Akonadi::Collection &resultCollection) Q_DECL_OVERRIDE;

private:
    QDomElement findElementByRid(const QString &rid) const;

    static QSet<qint64> parseQuery(const QString &queryString);

private Q_SLOTS:
    void load();
    void save();

private:
    Akonadi::XmlDocument mDocument;
    QFileSystemWatcher *mWatcher;
    KnutSettings *mSettings;
};

#endif
