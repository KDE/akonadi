/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QList>
#include <QSharedDataPointer>
#include <QSharedPointer>

#include "akonadicore_export.h"

namespace Akonadi
{
namespace Protocol
{
class ChangeNotification;
using ChangeNotificationPtr = QSharedPointer<ChangeNotification>;
}

class ChangeNotificationPrivate;

/**
 * Emitted by Monitor::debugNotification() signal.
 *
 * This is purely for debugging purposes and should never be used in regular
 * applications.
 *
 * @since 5.4
 */
class AKONADICORE_EXPORT ChangeNotification
{
public:
    enum Type {
        Items,
        Collection,
        Tag,
        Relation,
        Subscription,
    };

    explicit ChangeNotification();
    ChangeNotification(const ChangeNotification &other);
    ~ChangeNotification();

    ChangeNotification &operator=(const ChangeNotification &other);

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] QDateTime timestamp() const;
    void setTimestamp(const QDateTime &timestamp);

    [[nodiscard]] QList<QByteArray> listeners() const;
    void setListeners(const QList<QByteArray> &listeners);

    [[nodiscard]] Type type() const;
    void setType(Type type);

    [[nodiscard]] Protocol::ChangeNotificationPtr notification() const;
    void setNotification(const Protocol::ChangeNotificationPtr &ntf);

private:
    QSharedDataPointer<ChangeNotificationPrivate> d;
};

}
