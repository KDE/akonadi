/*
 * SPDX-FileCopyrightText: 2011-2025 Laurent Montel <montel@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "entitytreemodel.h"
#include "recentcollectionaction_p.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QAction>
#include <QMenu>
using namespace Akonadi;

static const int s_maximumRecentCollection = 10;

static QStringList readConfig()
{
    const KSharedConfig::Ptr akonadiConfig = KSharedConfig::openConfig(QStringLiteral("akonadikderc"));
    const KConfigGroup group(akonadiConfig, QStringLiteral("Recent Collections"));
    return (group.readEntry("Collections", QStringList()));
}

static void writeConfig(const QStringList &list)
{
    KSharedConfig::Ptr akonadiConfig = KSharedConfig::openConfig(QStringLiteral("akonadikderc"));
    KConfigGroup group(akonadiConfig, QStringLiteral("Recent Collections"));
    group.writeEntry("Collections", list);
    group.sync();
}

RecentCollectionAction::RecentCollectionAction(Akonadi::StandardActionManager::Type type,
                                               const Akonadi::Collection::List &selectedCollectionsList,
                                               const QAbstractItemModel *model,
                                               QMenu *menu)
    : QObject(menu)
    , mListRecentCollection(readConfig())
    , mMenu(menu)
    , mModel(model)
{
    mRecentAction = mMenu->addAction(i18n("Recent Folder"));
    mMenu->addSeparator();
    fillRecentCollection(type, selectedCollectionsList);
}

RecentCollectionAction::~RecentCollectionAction()
{
    //    if (needToDeleteMenu) {
    //        delete mRecentAction->menu();
    //    }
}

bool RecentCollectionAction::clear()
{
    delete mRecentAction->menu();
    needToDeleteMenu = false;
    if (mListRecentCollection.isEmpty()) {
        mRecentAction->setEnabled(false);
        return true;
    }
    return false;
}

void RecentCollectionAction::fillRecentCollection(Akonadi::StandardActionManager::Type type, const Akonadi::Collection::List &selectedCollectionsList)
{
    if (clear()) {
        return;
    }

    auto popup = new QMenu;
    mRecentAction->setMenu(popup);
    needToDeleteMenu = true;

    const int numberOfRecentCollection(mListRecentCollection.count());
    for (int i = 0; i < numberOfRecentCollection; ++i) {
        const QModelIndex index = Akonadi::EntityTreeModel::modelIndexForCollection(mModel, Akonadi::Collection(mListRecentCollection.at(i).toLongLong()));
        const auto collection = mModel->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (index.isValid()) {
            const bool collectionIsSelected = selectedCollectionsList.contains(collection);
            if (type == Akonadi::StandardActionManager::MoveCollectionToMenu && collectionIsSelected) {
                continue;
            }

            const bool canCreateNewItems = (collection.rights() & Collection::CanCreateItem);
            QAction *action = popup->addAction(actionName(index));
            const auto icon = mModel->data(index, Qt::DecorationRole).value<QIcon>();
            action->setIcon(icon);
            action->setData(QVariant::fromValue<QModelIndex>(index));
            action->setEnabled(canCreateNewItems);
        }
    }
}

QString RecentCollectionAction::actionName(QModelIndex index)
{
    QString name = index.data().toString();
    name.replace(QLatin1Char('&'), QStringLiteral("&&"));

    index = index.parent();
    QString topLevelName;
    while (index != QModelIndex()) {
        topLevelName = index.data().toString();
        index = index.parent();
    }
    if (topLevelName.isEmpty()) {
        return name;
    } else {
        topLevelName.replace(QLatin1Char('&'), QStringLiteral("&&"));
        return QStringLiteral("%1 - %2").arg(name, topLevelName);
    }
}

void RecentCollectionAction::addRecentCollection(Akonadi::StandardActionManager::Type type, Akonadi::Collection::Id id)
{
    mListRecentCollection = addRecentCollection(id);
    fillRecentCollection(type, Akonadi::Collection::List());
}

/* static */ QStringList RecentCollectionAction::addRecentCollection(Akonadi::Collection::Id id)
{
    QStringList listRecentCollection = readConfig();

    const QString newCollectionID = QString::number(id);
    if (listRecentCollection.contains(newCollectionID)) {
        // first() is safe to use if we get here
        if (listRecentCollection.first() == newCollectionID) {
            // already most recently used, nothing to do
            return (listRecentCollection);
        }

        listRecentCollection.removeAll(newCollectionID);
    }

    listRecentCollection.prepend(newCollectionID);
    while (listRecentCollection.count() > s_maximumRecentCollection) {
        listRecentCollection.removeLast();
    }

    writeConfig(listRecentCollection);
    return (listRecentCollection);
}

void RecentCollectionAction::cleanRecentCollection()
{
    mListRecentCollection.clear();
    writeConfig(mListRecentCollection);
    clear();
}

#include "moc_recentcollectionaction_p.cpp"
