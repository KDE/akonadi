/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "accountsintegration.h"
#include "accountsadaptor.h"
#include "config-akonadi.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QTimer>

#if WITH_ACCOUNTS
#include <Accounts/Account>
#include <Accounts/Manager>
#include <Core>
#include <GetCredentialsJob>
#endif

using namespace Akonadi;
using namespace std::chrono_literals;

AccountsIntegration::AccountsIntegration()
{
#if WITH_ACCOUNTS
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Accounts"), this);
    new Akonadi__AccountsAdaptor(this);
#endif
}

bool AccountsIntegration::isEnabled() const
{
#if WITH_ACCOUNTS
    return true;
#else
    return false;
#endif
}

std::optional<quint32> AccountsIntegration::accountId() const
{
    return mAccountId;
}

quint32 AccountsIntegration::getAccountId() const
{
    return mAccountId.has_value() ? *mAccountId : 0;
}

void AccountsIntegration::setAccountId(quint32 accountId)
{
    if (accountId <= 0) {
        mAccountId = std::nullopt;
    } else {
        mAccountId = accountId;
    }
    Q_EMIT accountChanged();
}

std::optional<QString> AccountsIntegration::accountName() const
{
#if WITH_ACCOUNTS
    if (!mAccountId.has_value()) {
        return std::nullopt;
    }

    auto const account = KAccounts::accountsManager()->account(mAccountId.value());
    if (!account) {
        return std::nullopt;
    }

    return account->displayName();
#else
    return {};
#endif
}

void AccountsIntegration::requestAuthData(const QString &serviceType, AuthDataCallback &&callback, ErrorCallback &&errCallback)
{
#if WITH_ACCOUNTS
    if (!mAccountId.has_value()) {
        QTimer::singleShot(0s, this, [error = std::move(errCallback)]() {
            error(i18n("There is currently no account configured."));
        });
        return;
    }

    auto job = new GetCredentialsJob(mAccountId.value(), this);
    job->setServiceType(serviceType);
    connect(job, &GetCredentialsJob::result, this, [job, callback = std::move(callback), error = std::move(errCallback)]() {
        if (job->error()) {
            error(job->errorString());
        } else {
            callback(job->credentialsData());
        }
    });
    job->start();
#else
    QTimer::singleShot(0s, this, [error = std::move(errCallback)]() {
        error(i18n("Accounts integration is not supported"));
    });
#endif
}
