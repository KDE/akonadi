#include "onlineaccountsintegration.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

using namespace Qt::Literals;

OnlineAccountsIntegration::OnlineAccountsIntegration()
    : QObject()
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

void OnlineAccountsIntegration::slotAccountCreationFinished(const QDBusObjectPath &path, const QString &xdgActivationToken)
{
    qWarning() << "account added" << path << xdgActivationToken;
}
