/***************************************************************************
 *   SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>     *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "onlineaccountsintegration.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>

#include "agenttype.h"

using namespace Qt::Literals;

OnlineAccountsIntegration::OnlineAccountsIntegration(AgentManager &manager)
    : QObject()
    , mAgentManager(manager)
{
    // register
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

    // list accounts
    QDBusMessage msg =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, u"/org/kde/KOnlineAccounts"_s, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    msg.setArguments({u"org.kde.KOnlineAccounts.Manager"_s, u"accounts"_s});
    QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(msg);

    if (!reply.isValid()) {
        qWarning() << "error" << reply.error();
    }
    const auto accounts = qdbus_cast<QList<QDBusObjectPath>>(reply.value().variant());

    qWarning() << "got accounts" << accounts;
}

void OnlineAccountsIntegration::slotAccountCreationFinished(const QDBusObjectPath &path, const QString & /*xdgActivationToken*/)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, path.path(), u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
    msg.setArguments({u"org.kde.KOnlineAccounts.Account"_s});
    QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(msg);

    if (!reply.isValid()) {
        qWarning() << "error" << reply.error();
    }

    const QStringList types = reply.value()[u"types"_s].toStringList();

    QSet<QString> requestedTypes;

    const auto agentTypes = mAgentManager.agentTypes();
    for (auto type : agentTypes) {
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

    for (auto type : requestedTypes) {
        const auto identifier = mAgentManager.createAgentInstance(type);

        const auto serviceName = Akonadi::DBus::agentServiceName(identifier, Akonadi::DBus::Resource);

        QDBusServiceWatcher *watcher = new QDBusServiceWatcher(serviceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);
        connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, [identifier, path](const QString &service) {
            qWarning() << "registered" << service;

            QDBusMessage msg = QDBusMessage::createMethodCall(u"org.freedesktop.Akonadi.Resource." + identifier,
                                                              u"/Accounts"_s,
                                                              u"org.kde.Akonadi.Accounts"_s,
                                                              u"setAccount"_s);
            msg.setArguments({path});

            QDBusReply<void> reply = QDBusConnection::sessionBus().call(msg);

            qWarning() << "reply" << reply.error();
        });
    }
}
