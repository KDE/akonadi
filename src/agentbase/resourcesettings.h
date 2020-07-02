/*
    SPDX-FileCopyrightText: 2010-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RESOURCESETTINGS_H
#define RESOURCESETTINGS_H

#include "akonadiagentbase_export.h"
#include "resourcebasesettings.h"

namespace Akonadi
{

class AKONADIAGENTBASE_EXPORT ResourceSettings : public Akonadi::ResourceBaseSettings //krazy:exclude=dpointer
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

#endif /* RESOURCESETTINGS_H */
