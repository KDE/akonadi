/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config_p.h"
#include "private/instance_p.h"

#include <KSharedConfig>
#include <KConfigGroup>

using namespace Akonadi;

Config Config::sConfig{};

namespace {

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
        const auto group = config->group(group_PayloadCompression);
        payloadCompression.enabled = group.readEntry(key_PC_Enabled, payloadCompression.enabled);
    }
}

void Config::setConfig(const Config &config)
{
    sConfig = config;
}
