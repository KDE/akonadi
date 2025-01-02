/*
    SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "accountactivitiesabstract.h"
using namespace Akonadi;
AccountActivitiesAbstract::AccountActivitiesAbstract(QObject *parent)
    : QObject{parent}
{
}

AccountActivitiesAbstract::~AccountActivitiesAbstract() = default;

#include "moc_accountactivitiesabstract.cpp"
