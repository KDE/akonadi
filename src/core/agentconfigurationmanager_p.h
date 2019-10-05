/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_AGENTCONFIGURATIOMANAGER_P_H
#define AKONADI_AGENTCONFIGURATIOMANAGER_P_H

#include <QObject>

#include "akonadicore_export.h"

namespace Akonadi {

class AKONADICORE_EXPORT AgentConfigurationManager : public QObject
{
    Q_OBJECT
public:
    static AgentConfigurationManager *self();
    ~AgentConfigurationManager() override;

    bool registerInstanceConfiguration(const QString &instance);
    void unregisterInstanceConfiguration(const QString &instance);

    bool isInstanceRegistered(const QString &instance) const;

    QString findConfigPlugin(const QString &type) const;

private:
    AgentConfigurationManager(QObject *parent = nullptr);

    class Private;
    friend class Private;
    QScopedPointer<Private> const d;
    static AgentConfigurationManager *sInstance;
};

}

#endif
