/*
    SPDX-FileCopyrightText: 2010-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiagentbase_export.h"
#include "resourcebasesettings.h"

namespace Akonadi
{
class AKONADIAGENTBASE_EXPORT ResourceSettings : public Akonadi::ResourceBaseSettings
{
    Q_OBJECT
public:
    static ResourceSettings *self();

private:
    ResourceSettings();
    ~ResourceSettings() override;
    static ResourceSettings *mSelf;
};

}
