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

#ifndef AKONADI_AGENTCONFIGURATIONBASE_H
#define AKONADI_AGENTCONFIGURATIONBASE_H

#include "akonadicore_export.h"
#include "agentconfigurationfactorybase.h"

#include <QObject>
#include <KSharedConfig>
#include <QDialogButtonBox>

class KAboutData;

namespace Akonadi {

/**
 * @brief Base class for configuration UI for Akonadi agents
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
 * The plugin binary should be installed into akonadi/config subdirectory in one
 * of the paths search by QCoreApplication::libraryPaths().
 */

class AKONADICORE_EXPORT AgentConfigurationBase : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a new AgentConfigurationBase objects.
     *
     * The @p parentWidget should be used as a parent widget for the configuration
     * widgets.
     *
     * Subclasses must provide a constructor with this exact signature.
     */
    explicit AgentConfigurationBase(const KSharedConfigPtr &config, QWidget *parentWidget,
                                    const QVariantList &args);

    ~AgentConfigurationBase() override;

    /**
     * Reimplement to load settings from the configuration object into widgets.
     *
     * @warning Always call the base class implementation at the beginning of
     * your overridden method!
     *
     * @see config(), save()
     */
    virtual void load();

    /**
     * Reimplement to save new settings into the configuration object.
     *
     * Return true if the configuration has been successfully saved and should
     * be applied to the agent, return false otherwise.
     *
     * @warning Always remember call the base class implementation at the end
     * of your overridden method!
     *
     * @see config(), load()
     */
    virtual bool save() const;

    /**
     * Returns about data for the currently configured component.
     *
     * May return a null pointer.
     */
    KAboutData *aboutData() const;

    /**
     * Reimplement to restore dialog size.
     */
    virtual QSize restoreDialogSize() const;

    /**
     * Reimplement to save dialog size.
     */
    virtual void saveDialogSize(QSize size);

    virtual QDialogButtonBox::StandardButtons standardButtons() const;

protected:
    QWidget *parentWidget() const;

    /**
     * Returns KConfig object belonging to the current Akonadi agent instance.
     */
    KSharedConfigPtr config() const;

    /**
     * Returns identifier of the Akonadi agent instance currently being configured.
     */
    QString identifier() const;

    /**
     * When KAboutData is provided the dialog will also contain KHelpMenu with
     * access to user help etc.
     */
    void setKAboutData(const KAboutData &aboutData);

Q_SIGNALS:
    void enableOkButton(bool enabled);

private:
    class Private;
    friend class Private;
    QScopedPointer<Private> d;
};

} // namespace

#endif // AKONADI_AGENTCONFIGURATIONBASE_H
