// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "agentwidgetbase.h"

#include <KIconTheme>
#include <KStyleManager>

Akonadi::AgentWidgetBase::AgentWidgetBase(const QString &id)
    : Akonadi::AgentBase(id)
{
}

void Akonadi::AgentWidgetBase::initTheming()
{
    KIconTheme::initTheme();
    KStyleManager::initStyle();

    QGuiApplication::setApplicationDisplayName(programName());
}
