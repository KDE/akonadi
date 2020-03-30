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

#ifndef AKONADI_ACCOUNTSINTEGRATION_H_
#define AKONADI_ACCOUNTSINTEGRATION_H_

#include <QObject>
#include "akonadiagentbase_export.h"

#include <functional>
#include <optional>

class Akonadi__AccountsAdaptor;
namespace Akonadi
{

class AKONADIAGENTBASE_EXPORT AccountsIntegration : public QObject
{
    Q_OBJECT

    friend class ::Akonadi__AccountsAdaptor;
public:
    explicit AccountsIntegration();
    ~AccountsIntegration() override = default;

    /**
     * Returns whether Accounts integration is enabled.
     */
    bool isEnabled() const;

    using AuthDataCallback = std::function<void(const QVariantMap &)>;
    using ErrorCallback = std::function<void(const QString &)>;
    void requestAuthData(const QString &serviceType, AuthDataCallback &&cb, ErrorCallback &&err);

    std::optional<QString> accountName() const;
public Q_SLOTS:
    std::optional<quint32> accountId() const;
    void setAccountId(quint32 accountId);

Q_SIGNALS:
    void accountChanged();

private:
    // For DBus adaptor which doesn't understand std::optional
    quint32 getAccountId() const;

    std::optional<quint32> mAccountId;
};



} // namespace Akonadi

#endif
