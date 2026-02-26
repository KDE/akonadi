/*
    SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "tagsfilterproxymodel.h"

using namespace Akonadi;

TagsFilterProxyModel::TagsFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
}

TagsFilterProxyModel::~TagsFilterProxyModel() = default;
#include "moc_tagsfilterproxymodel.cpp"
