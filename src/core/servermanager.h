/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QMetaType>
#include <QObject>

namespace Akonadi
{
class ServerManagerPrivate;

/**
 * @short Provides methods to control the Akonadi server process.
 *
 * Asynchronous, low-level control of the Akonadi server.
 * Akonadi::Control provides a synchronous interface to some of the methods in here.
 *
 * @author Volker Krause <vkrause@kde.org>
 * @see Akonadi::Control
 * @since 4.2
 */
class AKONADICORE_EXPORT ServerManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Enum for the various states the server can be in.
     * @since 4.5
     */
    enum State {
        NotRunning, ///< Server is not running, could be no one started it yet or it failed to start.
        Starting, ///< Server was started but is not yet running.
        Running, ///< Server is running and operational.
        Stopping, ///< Server is shutting down.
        Broken, ///< Server is not operational and an error has been detected.
        Upgrading ///< Server is performing a database upgrade as part of a new startup.
    };

    /**
     * Starts the server. This method returns immediately and does not wait
     * until the server is actually up and running.
     * @return @c true if the start was possible (which not necessarily means
     * the server is really running though) and @c false if an immediate error occurred.
     * @see Akonadi::Control::start()
     */
    static bool start();

    /**
     * Stops the server. This methods returns immediately after the shutdown
     * command has been send and does not wait until the server is actually
     * shut down.
     * @return @c true if the shutdown command was sent successfully, @c false
     * otherwise
     */
    static bool stop();

    /**
     * Shows the Akonadi self test dialog, which tests Akonadi for various problems
     * and reports these to the user if.
     * @param parent the parent widget for the dialog
     */
    static void showSelfTestDialog(QWidget *parent);

    /**
     * Checks if the server is available currently. For more detailed status information
     * see state().
     * @see state()
     */
    Q_REQUIRED_RESULT static bool isRunning();

    /**
     * Returns the state of the server.
     * @since 4.5
     */
    Q_REQUIRED_RESULT static State state();

    /**
     * Returns the reason why the Server is broken, if known.
     *
     * If state() is @p Broken, then you can use this method to obtain a more
     * detailed description of the problem and present it to users. Note that
     * the message can be empty if the reason is not known.
     *
     * @since 5.6
     */
    Q_REQUIRED_RESULT static QString brokenReason();

    /**
     * Returns the identifier of the Akonadi instance we are connected to. This is usually
     * an empty string (representing the default instance), unless you have explicitly set
     * the AKONADI_INSTANCE environment variable to connect to a different one.
     * @since 4.10
     */
    Q_REQUIRED_RESULT static QString instanceIdentifier();

    /**
     * Returns @c true if we are connected to a non-default Akonadi server instance.
     * @since 4.10
     */
    Q_REQUIRED_RESULT static bool hasInstanceIdentifier();

    /**
     * Types of known D-Bus services.
     * @since 4.10
     */
    enum ServiceType {
        Server,
        Control,
        ControlLock,
        UpgradeIndicator,
    };

    /**
     * Returns the namespaced D-Bus service name for @p serviceType.
     * Use this rather the raw service name strings in order to support usage of a non-default
     * instance of the Akonadi server.
     * @param serviceType the service type for which to return the D-Bus name
     * @since 4.10
     */
    static QString serviceName(ServiceType serviceType);

    /**
     * Known agent types.
     * @since 4.10
     */
    enum ServiceAgentType {
        Agent,
        Resource,
        Preprocessor,
    };

    /**
     * Returns the namespaced D-Bus service name for an agent of type @p agentType with agent
     * identifier @p identifier.
     * @param agentType the agent type to use for D-Bus base name
     * @param identifier the agent identifier to include in the D-Bus name
     * @since 4.10
     */
    Q_REQUIRED_RESULT static QString agentServiceName(ServiceAgentType agentType, const QString &identifier);

    /**
     * Adds the multi-instance namespace to @p string if required (with '_' as separator).
     * Use whenever a multi-instance safe name is required (configfiles, identifiers, ...).
     * @param string the string to adapt
     * @since 4.10
     */
    Q_REQUIRED_RESULT static QString addNamespace(const QString &string);

    /**
     * Returns the singleton instance of this class, for connecting to its
     * signals
     */
    static ServerManager *self();

    enum OpenMode {
        ReadOnly,
        ReadWrite,
    };
    /**
     * Returns absolute path to akonadiserverrc file with Akonadi server
     * configuration.
     */
    Q_REQUIRED_RESULT static QString serverConfigFilePath(OpenMode openMode);

    /**
     * Returns absolute path to configuration file of an agent identified by
     * given @p identifier.
     */
    Q_REQUIRED_RESULT static QString agentConfigFilePath(const QString &identifier);

    /**
     * Returns current Akonadi database generation identifier
     *
     * Generation is guaranteed to never change unless as long as the database
     * backend is not removed and re-created. In such case it is guaranteed that
     * the new generation number will be higher than the previous one.
     *
     * Generation can be used by applications to detect when Akonadi database
     * has been recreated and thus some of the configuration (for example
     * collection IDs stored in a config file) must be invalidated.
     *
     * @note Note that the generation number is only available if the server
     * is running. If this function is called before the server starts it will
     * return 0.
     *
     * @since 5.4
     */
    Q_REQUIRED_RESULT static uint generation();

Q_SIGNALS:
    /**
     * Emitted whenever the server becomes fully operational.
     */
    void started();

    /**
     * Emitted whenever the server becomes unavailable.
     */
    void stopped();

    /**
     * Emitted whenever the server state changes.
     * @param state the new server state
     * @since 4.5
     */
    void stateChanged(Akonadi::ServerManager::State state);

private:
    /// @cond PRIVATE
    friend class ServerManagerPrivate;
    ServerManager(ServerManagerPrivate *dd);
    ServerManagerPrivate *const d;
    /// @endcond
};

}

Q_DECLARE_METATYPE(Akonadi::ServerManager::State)

