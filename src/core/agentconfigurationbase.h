/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentconfigurationfactorybase.h"
#include "akonadicore_export.h"

#include <KSharedConfig>
#include <QObject>

#include <memory>

namespace Akonadi
{
class AgentConfigurationBasePrivate;

/*!
 * \brief Base class for configuration UI for Akonadi agents
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
 * \code
 * {
 *     "X-Akonadi-PluginType": "AgentConfig",
 *     "X-Akonadi-Library": "exampleresourceconfig",
 *     "X-Akonadi-AgentConfig-Type": "akonadi_example_resource"
 * }
 * \endcode
 *
 * The \a X-Akonadi-Library value must match the name of the plugin binary without
 * the (optional) "lib" prefix and file extension. The \a X-Akonadi-AgentConfig-Type
 * value must match the name of the \a X-Akonadi-Identifier value from the agent's
 * desktop file.
 *
 * The plugin binary should be installed into pim<version>/akonadi/config subdirectory in one
 * of the paths search by QCoreApplication::libraryPaths().
 *
 * \class Akonadi::AgentConfigurationBase
 * \inheaderfile Akonadi/AgentConfigurationBase
 * \inmodule AkonadiCore
 */
class AKONADICORE_EXPORT AgentConfigurationBase : public QObject
{
    Q_OBJECT
public:
    /*!
     * Creates a new AgentConfigurationBase objects.
     *
     * The \a parentWidget should be used as a parent widget for the configuration
     * widgets.
     *
     * Subclasses must provide a constructor with this exact signature.
     */
    explicit AgentConfigurationBase(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args);

    /*!
     * Destructor.
     */
    ~AgentConfigurationBase() override;

    /*!
     * Reimplement to load settings from the configuration object into widgets.
     *
     * \warning Always call the base class implementation at the beginning of
     * your overridden method!
     *
     * \sa config(), save()
     */
    virtual void load();

    /*!
     * Reimplement to save new settings into the configuration object.
     *
     * Return true if the configuration has been successfully saved and should
     * be applied to the agent, return false otherwise.
     *
     * \warning Always remember call the base class implementation at the end
     * of your overridden method!
     *
     * \sa config(), load()
     */
    virtual bool save() const;

    struct ActivitySettings {
        bool enabled = false;
        QStringList activities;
    };

    /*!
     * \brief saveActivitiesSettings
     * \a activities
     * save activities settings
     */
    void saveActivitiesSettings(const ActivitySettings &activities) const;

    /*!
     * \brief restoreActivitiesSettings
     * Returns activities settings
     */
    [[nodiscard]] ActivitySettings restoreActivitiesSettings() const;

protected:
    /*!
     * Returns the parent widget for the configuration UI.
     * \return The parent widget passed during construction.
     */
    QWidget *parentWidget() const;

    /*!
     * Returns the KConfig object belonging to the current Akonadi agent instance.
     * \return A KSharedConfigPtr to the agent's configuration object.
     */
    KSharedConfigPtr config() const;

    /*!
     * Returns the identifier of the Akonadi agent instance currently being configured.
     * \return The agent instance identifier as a string.
     */
    [[nodiscard]] QString identifier() const;

Q_SIGNALS:
    /*!
     * Emitted to control the enabled state of the OK button in the configuration dialog.
     * \a enabled True to enable the OK button, false to disable it.
     */
    void enableOkButton(bool enabled);

private:
    friend class AgentConfigurationBasePrivate;
    std::unique_ptr<AgentConfigurationBasePrivate> const d;
};

} // namespace
