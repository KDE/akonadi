/*
 *   SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "onlineaccountsintegration.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>

#include "accountinterface.h"
#include "agenttype.h"
#include "akonadicontrol_debug.h"

using namespace Qt::Literals;

OnlineAccountsIntegration::OnlineAccountsIntegration(AgentManager &manager)
    : QObject()
    , mAgentManager(manager)
{
    // register app
    QDBusMessage rm =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, u"/org/kde/KOnlineAccounts"_s, u"org.kde.KOnlineAccounts.Manager"_s, u"registerApp"_s);

    rm.setArguments({u"org.kde.akonadi"_s});
    QDBusConnection::sessionBus().call(rm);

    // watch for granted accounts
    bool ret = QDBusConnection::sessionBus().connect(u"org.kde.KOnlineAccounts"_s,
                                                     u"/org/kde/KOnlineAccounts"_s,
                                                     u"org.kde.KOnlineAccounts.Manager"_s,
                                                     u"accountAccessGranted"_s,
                                                     this,
                                                     SLOT(slotAccountCreationFinished(const QDBusObjectPath &, const QString &)));
    Q_ASSERT(ret);

    ret = QDBusConnection::sessionBus().connect(u"org.kde.KOnlineAccounts"_s,
                                                u"/org/kde/KOnlineAccounts"_s,
                                                u"org.kde.KOnlineAccounts.Manager"_s,
                                                u"accountRemoved"_s,
                                                this,
                                                SLOT(slotAccountRemoved(const QDBusObjectPath &)));
    Q_ASSERT(ret);

    // list accounts
    QDBusMessage msg =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, u"/org/kde/KOnlineAccounts"_s, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    msg.setArguments({u"org.kde.KOnlineAccounts.Manager"_s, u"accounts"_s});
    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(msg);

    if (!reply.isValid()) {
        qCWarning(AKONADICONTROL_LOG) << "Could not list online accounts:" << reply.error().message();
    }
    const auto accounts = qdbus_cast<QList<QDBusObjectPath>>(reply.value().variant());

    qCDebug(AKONADICONTROL_LOG) << "got accounts" << accounts;
}

void OnlineAccountsIntegration::slotAccountCreationFinished(const QDBusObjectPath &path, const QString & /*xdgActivationToken*/)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, path.path(), u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
    msg.setArguments({u"org.kde.KOnlineAccounts.Account"_s});
    QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(msg);

    if (!reply.isValid()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to get account properties for" << path << ":" << reply.error().message();
    }

    const QStringList types = reply.value()[u"types"_s].toStringList();

    qCDebug(AKONADICONTROL_LOG) << "Account has types" << types;

    QSet<QString> requestedTypes;

    const auto agentTypes = mAgentManager.agentTypes();
    for (const auto &type : agentTypes) {
        const auto supportedTypes = mAgentManager.agentCustomProperties(type)[u"KAccounts"_s].toStringList();
        if (types.contains(u"caldav") && supportedTypes.contains(u"dav-calendar")) {
            requestedTypes << type;
        }

        if (types.contains(u"carddav") && supportedTypes.contains(u"dav-contacts")) {
            requestedTypes << type;
        }

        if (types.contains(u"google") && supportedTypes.contains(u"google-contacts")) {
            requestedTypes << type;
        }
    }

    qCDebug(AKONADICONTROL_LOG) << "Creating agent instances" << requestedTypes;

    for (const auto &type : requestedTypes) {
        const auto identifier = mAgentManager.createAgentInstance(type);

        const auto serviceName = Akonadi::DBus::agentServiceName(identifier, Akonadi::DBus::Resource);

        QDBusServiceWatcher *watcher = new QDBusServiceWatcher(serviceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, [serviceName, path, watcher](const QString & /*service*/) {
            watcher->deleteLater();

            org::freedesktop::Akonadi::Agent::Account iface(serviceName, u"/Account"_s, QDBusConnection::sessionBus());

            QDBusPendingReply<void> reply = iface.setAccountId(path.path());
            reply.waitForFinished();

            if (!reply.isValid()) {
                qCWarning(AKONADICONTROL_LOG) << "Error when setting up setAccount" << reply.error().message();
            }
        });
    }
}

void OnlineAccountsIntegration::slotAccountRemoved(const QDBusObjectPath &path)
{
    const QStringList agents = mAgentManager.agentInstances();

    qCDebug(AKONADICONTROL_LOG) << "Removed account" << path;

    for (const QString &agentId : agents) {
        const auto serviceName = Akonadi::DBus::agentServiceName(agentId, Akonadi::DBus::Resource);
        org::freedesktop::Akonadi::Agent::Account iface(serviceName, u"/Account"_s, QDBusConnection::sessionBus());
        QDBusPendingReply<QDBusObjectPath> reply = iface.accountId();

        if (!reply.isValid()) {
            continue;
        }

        if (reply.value() == path) {
            qCDebug(AKONADICONTROL_LOG) << "Removing agent" << agentId;
            mAgentManager.removeAgentInstance(agentId);
        }
    }
}

#include "moc_onlineaccountsintegration.cpp"
