/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigViewStateSaver>

#include "collection.h"
#include "item.h"

#include "akonadiwidgets_export.h"

namespace Akonadi
{
class AKONADIWIDGETS_EXPORT ETMViewStateSaver : public KConfigViewStateSaver // krazy:exclude=dpointer
{
    Q_OBJECT
public:
    explicit ETMViewStateSaver(QObject *parent = nullptr);

    void selectCollections(const Akonadi::Collection::List &list);
    void selectCollections(const QList<Akonadi::Collection::Id> &list);
    void selectItems(const Akonadi::Item::List &list);
    void selectItems(const QList<Akonadi::Item::Id> &list);

    void setCurrentItem(const Akonadi::Item &item);
    void setCurrentCollection(const Akonadi::Collection &collection);

protected:
    /* reimp */
    QModelIndex indexFromConfigString(const QAbstractItemModel *model, const QString &key) const override;
    QString indexToConfigString(const QModelIndex &index) const override;
};

}

