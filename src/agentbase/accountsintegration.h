/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiagentbase_export.h"
#include <QObject>

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
    [[nodiscard]] bool isEnabled() const;

    using AuthDataCallback = std::function<void(const QVariantMap &)>;
    using ErrorCallback = std::function<void(const QString &)>;
    void requestAuthData(const QString &serviceType, AuthDataCallback &&cb, ErrorCallback &&err);

    std::optional<QString> accountName() const;
    std::optional<quint32> accountId() const;
public Q_SLOTS:
    void setAccountId(quint32 accountId);

Q_SIGNALS:
    void accountChanged();

private:
    // For DBus adaptor which doesn't understand std::optional
    AKONADIAGENTBASE_NO_EXPORT quint32 getAccountId() const;

    std::optional<quint32> mAccountId;
};

} // namespace Akonadi
