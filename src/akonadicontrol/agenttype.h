/*
    Copyright (c) 2007 - 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AGENTTYPE_H
#define AGENTTYPE_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QHash>

namespace Akonadi {
class ProcessControl;
}

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
    bool load(const QString &fileName, AgentManager *manager);
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
