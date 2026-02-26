/*
    SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>
namespace Akonadi
{
class TagsFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TagsFilterProxyModel(QObject *parent = nullptr);
    ~TagsFilterProxyModel() override;
};
}
