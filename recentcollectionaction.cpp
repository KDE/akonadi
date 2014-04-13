/*
 * Copyright (c) 2011 Laurent Montel <montel@kde.org>
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

#include "recentcollectionaction_p.h"
#include "metatypes.h"
#include <akonadi/entitytreemodel.h>
#include <akonadi/collectionmodel.h>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QMenu>
#include <QAction>
using namespace Akonadi;

static int s_maximumRecentCollection = 10;

RecentCollectionAction::RecentCollectionAction(Akonadi::StandardActionManager::Type type, const Akonadi::Collection::List &selectedCollectionsList,const QAbstractItemModel *model, QMenu *menu)
    : QObject(menu)
    , mMenu(menu)
    , mModel(model)
    , mRecentAction(0)
{
    mAkonadiConfig = KSharedConfig::openConfig(QLatin1String("akonadikderc"));
    KConfigGroup group(mAkonadiConfig, QLatin1String("Recent Collections"));

    mListRecentCollection = group.readEntry("Collections", QStringList());
    mRecentAction = mMenu->addAction(i18n("Recent Folder"));
    mMenu->addSeparator();
    fillRecentCollection(type, selectedCollectionsList);
}

RecentCollectionAction::~RecentCollectionAction()
{
}

void RecentCollectionAction::fillRecentCollection(Akonadi::StandardActionManager::Type type, const Akonadi::Collection::List &selectedCollectionsList)
{
    delete mRecentAction->menu();
    if (mListRecentCollection.isEmpty()) {
        mRecentAction->setEnabled(false);
        return;
    }

    QMenu *popup = new QMenu;
    mRecentAction->setMenu(popup);

    const int numberOfRecentCollection(mListRecentCollection.count());
    for (int i = 0; i < numberOfRecentCollection; ++i) {
        const QModelIndex index = Akonadi::EntityTreeModel::modelIndexForCollection(mModel, Akonadi::Collection(mListRecentCollection.at(i).toLongLong()));
        const Akonadi::Collection collection = mModel->data(index, Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();
        if (index.isValid()) {
            const bool collectionIsSelected = selectedCollectionsList.contains(collection);
            if (type == MoveCollectionToMenu && collectionIsSelected) {
               continue;
            }

            const bool canCreateNewItems = (collection.rights() &Collection::CanCreateItem);
            QAction *action = popup->addAction(actionName(index));
            const QIcon icon = mModel->data(index, Qt::DecorationRole).value<QIcon>();
            action->setIcon(icon);
            action->setData(QVariant::fromValue<QModelIndex>(index));
            action->setEnabled(canCreateNewItems);
        }
    }
}

QString RecentCollectionAction::actionName(QModelIndex index)
{
    QString name = index.data().toString();
    name.replace(QLatin1String("&"), QLatin1String("&&"));

    index = index.parent();
    QString topLevelName;
    while (index != QModelIndex()) {
        topLevelName = index.data().toString();
        index = index.parent();
    }
    if (topLevelName.isEmpty()) {
        return QString::fromLatin1("%1").arg(name);
    } else {
        topLevelName.replace(QLatin1String("&"), QLatin1String("&&"));
        return QString::fromLatin1("%1 - %2").arg(name).arg(topLevelName);
    }
}

void RecentCollectionAction::addRecentCollection(Akonadi::Collection::Id id)
{
    const QString newCollectionID = QString::number(id);
    if (mListRecentCollection.isEmpty() ||
        !mListRecentCollection.contains(newCollectionID)) {
        if (mListRecentCollection.count() == s_maximumRecentCollection) {
            mListRecentCollection.removeFirst();
        }
        mListRecentCollection.append(newCollectionID);
        writeConfig();
        fillRecentCollection();
    }
}

void RecentCollectionAction::writeConfig()
{
    KConfigGroup group(mAkonadiConfig, QLatin1String("Recent Collections"));
    group.writeEntry("Collections", mListRecentCollection);
    group.sync();
}

void RecentCollectionAction::cleanRecentCollection()
{
    mListRecentCollection.clear();
    writeConfig();
    fillRecentCollection();
}

#include "moc_recentcollectionaction_p.cpp"
