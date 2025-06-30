// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2022 Claudio Cambra <claudio.cambra@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/EntityTreeModel>
#include <QColor>

/// Despite the name, this handles the presentation of collections including display text and icons, not just colors.
class ColorProxyModel : public Akonadi::CollectionFilterProxyModel
{
    Q_OBJECT

public:
    enum Roles {
        IsResourceRole = Akonadi::EntityTreeModel::UserRole + 1,
    };
    Q_ENUM(Roles)

    explicit ColorProxyModel(QObject *parent = nullptr);
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

    [[nodiscard]] QColor getCollectionColor(Akonadi::Collection collection) const;
    [[nodiscard]] QColor color(Akonadi::Collection::Id collectionId) const;
    void setColor(Akonadi::Collection::Id collectionId, const QColor &color);

    void setStandardCollectionId(Akonadi::Collection::Id standardCollectionId);

private:
    mutable QHash<Akonadi::Collection::Id, QColor> colorCache;
    Akonadi::Collection::Id m_standardCollectionId = -1;
};
