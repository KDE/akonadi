// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "akonadicore_export.h"

#include <KSharedConfig>
#include <QObject>

namespace Akonadi
{
/*!
    \class Akonadi::AgentConfigurationQuickBase
    \inheaderfile Akonadi/AgentConfigurationQuickBase
    \inmodule AkonadiCore
    \brief Backend for a QML agent configuration page.

    Available to the page as \c configuration.controller.
*/
class AKONADICORE_EXPORT AgentConfigurationQuickBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString identifier READ identifier CONSTANT)
public:
    AgentConfigurationQuickBase(const KSharedConfigPtr &config, const QString &identifier, QObject *parent = nullptr);
    ~AgentConfigurationQuickBase() override;

    [[nodiscard]] QString identifier() const;
    Q_INVOKABLE virtual void load();
    Q_INVOKABLE virtual bool save();

Q_SIGNALS:
    void enableOkButton(bool enabled);

protected:
    [[nodiscard]] KSharedConfigPtr config() const;

private:
    KSharedConfigPtr m_config;
    QString m_identifier;
};

/*!
    \class Akonadi::AgentConfigurationQuickFactory
    \inheaderfile Akonadi/AgentConfigurationQuickBase
    \inmodule AkonadiCore
    \brief Factory for QML agent configuration plugins.

    Uses the widget plugin directory and IID. Metadata requires
    \c X-Akonadi-PluginType = \c AgentConfig,
    \c X-Akonadi-AgentConfig-Type = the agent type identifier, and
    \c X-Akonadi-ConfigUi = \c Quick.

    The page must be a Kirigami.Page with a required \c configuration property.
    Derive from org.kde.akonadi.AgentConfigurationPage for Save and Cancel buttons.
*/
class AKONADICORE_EXPORT AgentConfigurationQuickFactory : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~AgentConfigurationQuickFactory() override = default;

    struct Page {
        QString module;
        QString component;
    };

    /*! Returns the page's QML module URI and component name. */
    [[nodiscard]] virtual Page page() const = 0;
    [[nodiscard]] virtual AgentConfigurationQuickBase *create(const KSharedConfigPtr &config, const QString &identifier, QObject *parent) const = 0;
};
}

#define AKONADI_AGENTCONFIG_QUICK_FACTORY(FactoryName, metadata, Controller, Module, Component)                                                                \
    class FactoryName : public Akonadi::AgentConfigurationQuickFactory                                                                                         \
    {                                                                                                                                                          \
        Q_OBJECT                                                                                                                                               \
        Q_PLUGIN_METADATA(IID "org.freedesktop.Akonadi.AgentConfig" FILE metadata)                                                                             \
    public:                                                                                                                                                    \
        using Akonadi::AgentConfigurationQuickFactory::AgentConfigurationQuickFactory;                                                                         \
        Page page() const override                                                                                                                             \
        {                                                                                                                                                      \
            return {QStringLiteral(Module), QStringLiteral(Component)};                                                                                        \
        }                                                                                                                                                      \
        Akonadi::AgentConfigurationQuickBase *create(const KSharedConfigPtr &config, const QString &identifier, QObject *parent) const override                \
        {                                                                                                                                                      \
            return new Controller(config, identifier, parent);                                                                                                 \
        }                                                                                                                                                      \
    };
