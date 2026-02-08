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

/*!
 * \brief Provides methods to control the Akonadi server process.
 *
 * Asynchronous, low-level control of the Akonadi server.
 * Akonadi::Control provides a synchronous interface to some of the methods in here.
 *
 * \class Akonadi::ServerManager
 * \inheaderfile Akonadi/ServerManager
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 * \sa Akonadi::Control
 * \since 4.2
 */
class AKONADICORE_EXPORT ServerManager : public QObject
{
    Q_OBJECT
public:
    /*!
     * Enum for the various states the server can be in.
     * \since 4.5
     */
    enum State {
        NotRunning, ///< Server is not running, could be no one started it yet or it failed to start.
        Starting, ///< Server was started but is not yet running.
        Running, ///< Server is running and operational.
        Stopping, ///< Server is shutting down.
        Broken, ///< Server is not operational and an error has been detected.
        Upgrading ///< Server is performing a database upgrade as part of a new startup.
    };

    /*!
     * Starts the server. This method returns immediately and does not wait
     * until the server is actually up and running.
     * Returns \\ true if the start was possible (which not necessarily means
     * the server is really running though) and \\ false if an immediate error occurred.
     * \sa Akonadi::Control::start()
     */
    static bool start();

    /*!
     * Stops the server. This methods returns immediately after the shutdown
     * command has been send and does not wait until the server is actually
     * shut down.
     * Returns \\ true if the shutdown command was sent successfully, \\ false
     * otherwise
     */
    static bool stop();

    /*!
     * Shows the Akonadi self test dialog, which tests Akonadi for various problems
     * and reports these to the user if.
     * \a parent the parent widget for the dialog
     */
    static void showSelfTestDialog(QWidget *parent);

    /*!
     * Checks if the server is available currently. For more detailed status information
     * see state().
     * \sa state()
     */
    [[nodiscard]] static bool isRunning();

    /*!
     * Returns the state of the server.
     * \since 4.5
     */
    [[nodiscard]] static State state();

    /*!
     * Returns the reason why the Server is broken, if known.
     *
     * If state() is \a Broken, then you can use this method to obtain a more
     * detailed description of the problem and present it to users. Note that
     * the message can be empty if the reason is not known.
     *
     * \since 5.6
     */
    [[nodiscard]] static QString brokenReason();

    /*!
     * Returns the identifier of the Akonadi instance we are connected to. This is usually
     * an empty string (representing the default instance), unless you have explicitly set
     * the AKONADI_INSTANCE environment variable to connect to a different one.
     * \since 4.10
     */
    [[nodiscard]] static QString instanceIdentifier();

    /*!
     * Returns \\ true if we are connected to a non-default Akonadi server instance.
     * \since 4.10
     */
    [[nodiscard]] static bool hasInstanceIdentifier();

    /*!
     * Types of known D-Bus services.
     * \since 4.10
     */
    enum ServiceType {
        Server,
        Control,
        ControlLock,
        UpgradeIndicator,
    };

    /*!
     * Returns the namespaced D-Bus service name for \a serviceType.
     * Use this rather the raw service name strings in order to support usage of a non-default
     * instance of the Akonadi server.
     * \a serviceType the service type for which to return the D-Bus name
     * \since 4.10
     */
    static QString serviceName(ServiceType serviceType);

    /*!
     * Known agent types.
     * \since 4.10
     */
    enum ServiceAgentType {
        Agent,
        Resource,
        Preprocessor,
    };

    /*!
     * Returns the namespaced D-Bus service name for an agent of type \a agentType with agent
     * identifier \a identifier.
     * \a agentType the agent type to use for D-Bus base name
     * \a identifier the agent identifier to include in the D-Bus name
     * \since 4.10
     */
    [[nodiscard]] static QString agentServiceName(ServiceAgentType agentType, const QString &identifier);

    /*!
     * Adds the multi-instance namespace to \a string if required (with '_' as separator).
     * Use whenever a multi-instance safe name is required (configfiles, identifiers, ...).
     * \a string the string to adapt
     * \since 4.10
     */
    [[nodiscard]] static QString addNamespace(const QString &string);

    /*!
     * Returns the singleton instance of this class, for connecting to its
     * signals
     */
    static ServerManager *self();

    enum OpenMode {
        ReadOnly,
        ReadWrite,
    };
    /*!
     * Returns absolute path to akonadiserverrc file with Akonadi server
     * configuration.
     */
    [[nodiscard]] static QString serverConfigFilePath(OpenMode openMode);

    /*!
     * Returns absolute path to configuration file of an agent identified by
     * given \a identifier.
     */
    [[nodiscard]] static QString agentConfigFilePath(const QString &identifier);

    /*!
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
     * \
ote Note that the generation number is only available if the server
     * is running. If this function is called before the server starts it will
     * return 0.
     *
     * \since 5.4
     */
    [[nodiscard]] static uint generation();

Q_SIGNALS:
    /*!
     * Emitted whenever the server becomes fully operational.
     */
    void started();

    /*!
     * Emitted whenever the server becomes unavailable.
     */
    void stopped();

    /*!
     * Emitted whenever the server state changes.
     * \a state the new server state
     * \since 4.5
     */
    void stateChanged(Akonadi::ServerManager::State state);

private:
    friend class ServerManagerPrivate;
    ServerManager(ServerManagerPrivate *dd);
    ServerManagerPrivate *const d;
};

}

Q_DECLARE_METATYPE(Akonadi::ServerManager::State)
