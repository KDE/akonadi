// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <Akonadi/AgentInstance>
#include <QObject>
#include <QPluginLoader>
#include <QQmlComponent>
#include <QQmlEngine>
#include <qqmlregistration.h>

namespace Akonadi
{
class AgentConfigurationQuickBase;
}

class AgentConfigurationSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by AgentConfiguration")
    Q_PROPERTY(QQmlComponent *page READ page CONSTANT)
    Q_PROPERTY(QObject *controller READ controller CONSTANT)
    Q_PROPERTY(bool canSave READ canSave NOTIFY canSaveChanged)
public:
    AgentConfigurationSession(const Akonadi::AgentInstance &instance, QQmlEngine *engine, bool removeOnCancel, QObject *parent = nullptr);
    ~AgentConfigurationSession() override;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QQmlComponent *page() const;
    [[nodiscard]] QObject *controller() const;
    [[nodiscard]] bool canSave() const;

    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void canSaveChanged();
    void finished();

private:
    Akonadi::AgentInstance m_instance;
    QPluginLoader m_loader;
    QQmlComponent *m_page = nullptr;
    Akonadi::AgentConfigurationQuickBase *m_controller = nullptr;
    bool m_registered = false;
    bool m_removeOnCancel = false;
    bool m_canSave = true;
    bool m_finished = false;
};
