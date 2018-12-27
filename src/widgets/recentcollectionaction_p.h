/*
 * Copyright (C) 2011-2019 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */
#ifndef RECENTCOLLECTIONACTION_P_H
#define RECENTCOLLECTIONACTION_P_H

#include <QStringList>
#include <QModelIndex>
#include "collection.h"
#include <ksharedconfig.h>
#include <standardactionmanager.h>

class QMenu;
class QAbstractItemModel;
class QAction;
/**
 * @short A class to manage recent selected folder.
 *
 * @author Montel Laurent <montel@kde.org>
 * @since 4.8
 */

namespace Akonadi
{
class RecentCollectionAction : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a new collection recent action
     */
    explicit RecentCollectionAction(Akonadi::StandardActionManager::Type type, const Akonadi::Collection::List &selectedCollectionsList, const QAbstractItemModel *model, QMenu *menu);
    /**
     * Destroys the collection recent action.
     */
    ~RecentCollectionAction();

    /**
     * Add new collection. Will create a new item.
     */
    void addRecentCollection(StandardActionManager::Type type, Akonadi::Collection::Id id);

    void cleanRecentCollection();

private:
    void writeConfig();
    void fillRecentCollection(Akonadi::StandardActionManager::Type type, const Akonadi::Collection::List &selectedCollectionsList);
    QString actionName(QModelIndex index);
    bool clear();

private:
    QStringList mListRecentCollection;
    QMenu *mMenu = nullptr;
    const QAbstractItemModel *mModel = nullptr;
    QAction *mRecentAction = nullptr;
    KSharedConfig::Ptr mAkonadiConfig;
};
}

#endif /* RECENTCOLLECTIONACTION_P_H */
