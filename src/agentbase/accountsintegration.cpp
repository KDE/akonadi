/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#include "accountsintegration.h"
#include "accountsadaptor.h"
#include "config-akonadi.h"

#include <KLocalizedString>

#include <QTimer>
#include <QDBusConnection>

#ifdef WITH_ACCOUNTS
#include <KAccounts/GetCredentialsJob>
#include <KAccounts/Core>
#include <Accounts/Manager>
#include <Accounts/Account>
#endif

using namespace Akonadi;
using namespace std::chrono_literals;

AccountsIntegration::AccountsIntegration()
    : QObject()
{
#ifdef WITH_ACCOUNTS
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Accounts"), this);
    new Akonadi__AccountsAdaptor(this);
#endif
}

bool AccountsIntegration::isEnabled() const
{
#ifdef WITH_ACCOUNTS
    return true;
#else
    return false;
#endif
}

akOptional<quint32> AccountsIntegration::accountId() const
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
        mAccountId = nullopt;
    } else {
        mAccountId = accountId;
    }
    Q_EMIT accountChanged();
}

akOptional<QString> AccountsIntegration::accountName() const
{
#ifdef WITH_ACCOUNTS
    if (!mAccountId.has_value()) {
        return nullopt;
    }

    const auto account = KAccounts::accountsManager()->account(mAccountId.value());
    if (!account) {
        return nullopt;
    }

    return account->displayName();
#else
    return {};
#endif
}

void AccountsIntegration::requestAuthData(const QString &serviceType, AuthDataCallback &&callback, ErrorCallback &&errCallback)
{
#ifdef WITH_ACCOUNTS
    if (!mAccountId.has_value()) {
        QTimer::singleShot(0s, this, [error = std::move(errCallback)]() {
            error(i18n("There is currently no account configured."));
        });
        return;
    }

    auto job = new GetCredentialsJob(mAccountId.value(), this);
    job->setServiceType(serviceType);
    connect(job, &GetCredentialsJob::result,
            this, [job, callback = std::move(callback), error = std::move(errCallback)]() {
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
