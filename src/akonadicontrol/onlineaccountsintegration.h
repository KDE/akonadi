#pragma once

#include <QDBusObjectPath>
#include <QObject>

class OnlineAccountsIntegration : public QObject
{
    Q_OBJECT

public:
    OnlineAccountsIntegration();

    Q_SLOT void slotAccountCreationFinished(const QDBusObjectPath &path, const QString &xdgActivationToken);
};
