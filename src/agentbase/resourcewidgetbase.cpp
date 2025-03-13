// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "resourcewidgetbase.h"

#include <KIconTheme>
#include <KStyleManager>

Akonadi::ResourceWidgetBase::ResourceWidgetBase(const QString &id)
    : Akonadi::ResourceBase(id)
{
}

void Akonadi::ResourceWidgetBase::initTheming()
{
    KIconTheme::initTheme();
    KStyleManager::initStyle();

    QGuiApplication::setApplicationDisplayName(programName());
}
