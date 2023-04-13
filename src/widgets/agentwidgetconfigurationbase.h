/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include <akonadi/agentconfigurationbase.h>
#include <akonadi/agentconfigurationfactorybase.h>

#include <KSharedConfig>
#include <QDialogButtonBox>
#include <QObject>

#include <memory>

class KAboutData;

namespace Akonadi
{
class AgentWidgetConfigurationBasePrivate;

/**
 * @brief Base class for QWidget-based configuration UI for Akonadi agents
 *
 * Each agent that has a graphical configuration should subclass this class
 * and create its configuration UI there.
 *
 * The subclass must reimplement load() and save() virtual methods which
 * are called automatically. The load() method is called on start to initialize
 * widgets (thus subclasses don't need to call it themselves) or when user
 * clicks a "Reset" button. The save() method is called whenever user decides to
 * save changes.
 *
 * Since each Akonadi agent instance has its own configuration file whose
 * location and name is opaque to the implementation, config() method can be
 * used to get access to the current configuration object.
 *
 * The widget will not run in the same process as the Akonadi agent, thus all
 * communication with the resource (if needed) should be done over DBus. The
 * identifier of the instance currently being configured is accessible from the
 * identifier() method.
 *
 * There is no need to signal back to the resource when configuration is changed.
 * When save() is called and the dialog is destroyed, Akonadi will automatically
 * call AgentBase::reconfigure() in the respective Akonadi agent instance.
 *
 * It is guaranteed that only a single instance of the configuration dialog for
 * given agent will be opened at the same time.
 *
 * Subclasses of ConfigurationBase must be registered as Akonadi plugins using
 * AKONADI_AGENTCONFIG_FACTORY macro.
 *
 * The metadata JSON file then must contain the following values:
 * @code
 * {
 *     "X-Akonadi-PluginType": "AgentConfig",
 *     "X-Akonadi-Library": "exampleresourceconfig",
 *     "X-Akonadi-AgentConfig-Type": "akonadi_example_resource"
 * }
 * @endcode
 *
 * The @p X-Akonadi-Library value must match the name of the plugin binary without
 * the (optional) "lib" prefix and file extension. The @p X-Akonadi-AgentConfig-Type
 * value must match the name of the @p X-Akonadi-Identifier value from the agent's
 * desktop file.
 *
 * The plugin binary should be installed into pim<version>/akonadi/config subdirectory in one
 * of the paths searched by QCoreApplication::libraryPaths().
 */

class AKONADIWIDGETS_EXPORT AgentWidgetConfigurationBase : public AgentConfigurationBase
{
    Q_OBJECT
public:
    /**
     * Creates a new AgentWidgetsConfigurationBase objects.
     *
     * The @p parentWidget should be used as a parent widget for the configuration
     * widgets.
     *
     * Subclasses must provide a constructor with this exact signature.
     */
    explicit AgentWidgetConfigurationBase(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args);

    ~AgentWidgetConfigurationBase() override;

    /**
     * Reimplement to restore dialog size.
     */
    virtual QSize restoreDialogSize() const;

    /**
     * Reimplement to save dialog size.
     */
    virtual void saveDialogSize(const QSize &size);

    virtual QDialogButtonBox::StandardButtons standardButtons() const;

protected:
    QWidget *parentWidget() const;

private:
    friend class AgentWidgetConfigurationBasePrivate;
    std::unique_ptr<AgentWidgetConfigurationBasePrivate> const d;
};

} // namespace
