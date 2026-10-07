// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "knutconfigquick.h"

#include <QDir>

KnutConfigQuick::KnutConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent)
    : AgentConfigurationQuickBase(config, identifier, parent)
    , m_settings(config)
{
}

QString KnutConfigQuick::dataFile() const
{
    return m_dataFile;
}

void KnutConfigQuick::setDataFile(const QString &dataFile)
{
    if (m_dataFile == dataFile) {
        return;
    }
    m_dataFile = dataFile;
    Q_EMIT dataFileChanged();
}

bool KnutConfigQuick::fileWatchingEnabled() const
{
    return m_fileWatchingEnabled;
}

void KnutConfigQuick::setFileWatchingEnabled(bool enabled)
{
    if (m_fileWatchingEnabled == enabled) {
        return;
    }
    m_fileWatchingEnabled = enabled;
    Q_EMIT fileWatchingEnabledChanged();
}

void KnutConfigQuick::setDataFileUrl(const QUrl &url)
{
    setDataFile(url.toLocalFile());
}

void KnutConfigQuick::load()
{
    AgentConfigurationQuickBase::load();
    m_settings.load();
    setDataFile(m_settings.dataFile().isEmpty() ? QDir::homePath() : m_settings.dataFile());
    setFileWatchingEnabled(m_settings.fileWatchingEnabled());
}

bool KnutConfigQuick::save()
{
    m_settings.setDataFile(m_dataFile);
    m_settings.setFileWatchingEnabled(m_fileWatchingEnabled);
    m_settings.save();
    return AgentConfigurationQuickBase::save();
}

AKONADI_AGENTCONFIG_QUICK_FACTORY(KnutConfigQuickFactory, "knutconfigquick.json", KnutConfigQuick, "org.kde.akonadi.knutconfig", "ConfigurationPage")

#include "knutconfigquick.moc"
