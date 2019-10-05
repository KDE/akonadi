/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONMODEL_P_H
#define AKONADI_COLLECTIONMODEL_P_H

#include "collection.h"

#include <KLocalizedString>

#include <QHash>
#include <QModelIndex>
#include <QStringList>
#include <QIcon>

class KJob;

namespace Akonadi
{

class CollectionModel;
class CollectionStatistics;
class Monitor;
class Session;

/**
 * @internal
 */
class CollectionModelPrivate
{
public:
    Q_DECLARE_PUBLIC(CollectionModel)
    explicit CollectionModelPrivate(CollectionModel *parent)
        : q_ptr(parent)
        , headerContent(i18nc("@title:column, name of a thing", "Name"))
    {
    }

    virtual ~CollectionModelPrivate()
    {
    }

    CollectionModel *q_ptr;
    QHash<Collection::Id, Collection> collections;
    QHash<Collection::Id, QVector<Collection::Id> > childCollections;

    QHash<Collection::Id, Collection> m_newCollections;
    QHash< Collection::Id, QVector<Collection::Id> > m_newChildCollections;

    Monitor *monitor = nullptr;
    Session *session = nullptr;
    QStringList mimeTypes;
    bool fetchStatistics = false;
    bool unsubscribed = false;
    QString headerContent;

    void init();
    void startFirstListJob();
    void collectionRemoved(const Akonadi::Collection &collection);
    void collectionChanged(const Akonadi::Collection &collection);
    void updateDone(KJob *job);
    void collectionStatisticsChanged(Collection::Id, const Akonadi::CollectionStatistics &statistics);
    void listDone(KJob *job);
    void editDone(KJob *job);
    void dropResult(KJob *job);
    void collectionsChanged(const Akonadi::Collection::List &cols);

    QIcon iconForCollection(const Collection &collection) const;

    QModelIndex indexForId(Collection::Id id, int column = 0) const;
    bool removeRowFromModel(int row, const QModelIndex &parent = QModelIndex());
    bool supportsContentType(const QModelIndex &index, const QStringList &contentTypes);

private:
    // FIXME: This cache is a workaround for extremely slow QIcon::fromTheme()
    // caused by bottleneck in FrameworkIntegration. See bug #346644 for details.
    mutable QHash<QString, QIcon> mIconCache;
    mutable QString mIconThemeName;

    void updateSupportedMimeTypes(const Collection &col)
    {
        const QStringList l = col.contentMimeTypes();
        QStringList::ConstIterator constEnd(l.constEnd());
        for (QStringList::ConstIterator it = l.constBegin(); it != constEnd; ++it) {
            if ((*it) == Collection::mimeType()) {
                continue;
            }
            if (!mimeTypes.contains(*it)) {
                mimeTypes << *it;
            }
        }
    }
};

}

#endif
