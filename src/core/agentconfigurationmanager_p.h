/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include "akonadicore_export.h"

#include <memory>

namespace Akonadi
{
class AgentConfigurationManagerPrivate;

class AKONADICORE_EXPORT AgentConfigurationManager : public QObject
{
    Q_OBJECT
public:
    static AgentConfigurationManager *self();
    ~AgentConfigurationManager() override;

    bool registerInstanceConfiguration(const QString &instance);
    void unregisterInstanceConfiguration(const QString &instance);

    [[nodiscard]] bool isInstanceRegistered(const QString &instance) const;

    QString findConfigPlugin(const QString &type) const;

private:
    AgentConfigurationManager(QObject *parent = nullptr);

    friend class AgentConfigurationManagerPrivate;
    std::unique_ptr<AgentConfigurationManagerPrivate> const d;
    static AgentConfigurationManager *sInstance;
};

}
