/*
 * SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "collection.h"
#include <KSharedConfig>
#include <QModelIndex>
#include <QStringList>
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
    explicit RecentCollectionAction(Akonadi::StandardActionManager::Type type,
                                    const Akonadi::Collection::List &selectedCollectionsList,
                                    const QAbstractItemModel *model,
                                    QMenu *menu);
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
    QMenu *const mMenu;
    const QAbstractItemModel *mModel = nullptr;
    QAction *mRecentAction = nullptr;
    KSharedConfig::Ptr mAkonadiConfig;
    bool needToDeleteMenu = false;
};
}

