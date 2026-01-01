/*
    SPDX-FileCopyrightText: 2010-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourcesettings.h"

using namespace Akonadi;

ResourceSettings *ResourceSettings::mSelf = nullptr;

ResourceSettings *ResourceSettings::self()
{
    if (!mSelf) {
        mSelf = new ResourceSettings();
        mSelf->load();
    }

    return mSelf;
}

ResourceSettings::ResourceSettings()
{
}

ResourceSettings::~ResourceSettings()
{
}

#include "moc_resourcesettings.cpp"
