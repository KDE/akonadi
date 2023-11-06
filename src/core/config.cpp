/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config_p.h"
#include "private/instance_p.h"

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Akonadi;

Q_GLOBAL_STATIC(Config, sConfig) // NOLINT(readability-redundant-member-init)

namespace
{
QString getConfigName()
{
    if (Instance::hasIdentifier()) {
        return QStringLiteral("akonadi_%1rc").arg(Instance::identifier());
    } else {
        return QStringLiteral("akonadirc");
    }
}

static constexpr char group_PayloadCompression[] = "PayloadCompression";

// Payload compression
static constexpr char key_PC_Enabled[] = "enabled";

} // namespace

Config::Config()
{
    auto config = KSharedConfig::openConfig(getConfigName());

    {
        const auto group = config->group(QLatin1String(group_PayloadCompression));
        payloadCompression.enabled = group.readEntry(key_PC_Enabled, payloadCompression.enabled);
    }
}

const Config &Config::get()
{
    return *sConfig;
}
