/*
    SPDX-FileCopyrightText: 2007-2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AGENTTYPE_H
#define AGENTTYPE_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QHash>

class AgentManager;
class QSettings;

class AgentType
{
public:
    enum LaunchMethod {
        Process, /// Standalone executable
        Server,  /// Agent plugin launched in AgentManager
        Launcher /// Agent plugin launched in own process
    };

public:
    AgentType();
    Q_REQUIRED_RESULT bool load(const QString &fileName, AgentManager *manager);
    void save(QSettings *config) const;

    QString identifier;
    QString name;
    QString comment;
    QString icon;
    QStringList mimeTypes;
    QStringList capabilities;
    QString exec;
    QVariantMap custom;
    uint instanceCounter;
    LaunchMethod launchMethod;

    static const QLatin1String CapabilityUnique;
    static const QLatin1String CapabilityResource;
    static const QLatin1String CapabilityAutostart;
    static const QLatin1String CapabilityPreprocessor;
    static const QLatin1String CapabilitySearch;

private:
    QString readString(const QSettings &file, const QString &key);
};

#endif
