/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CHANGENOTIFICATION_H
#define AKONADI_CHANGENOTIFICATION_H

#include <QDateTime>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QVector>

#include <akonadicore_export.h>

namespace Akonadi
{
namespace Protocol
{
class ChangeNotification;
using ChangeNotificationPtr = QSharedPointer<ChangeNotification>;
}

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
    enum Type { Items, Collection, Tag, Relation, Subscription };

    explicit ChangeNotification();
    ChangeNotification(const ChangeNotification &other);
    ~ChangeNotification();

    ChangeNotification &operator=(const ChangeNotification &other);

    Q_REQUIRED_RESULT bool isValid() const;

    Q_REQUIRED_RESULT QDateTime timestamp() const;
    void setTimestamp(const QDateTime &timestamp);

    Q_REQUIRED_RESULT QVector<QByteArray> listeners() const;
    void setListeners(const QVector<QByteArray> &listeners);

    Q_REQUIRED_RESULT Type type() const;
    void setType(Type type);

    Q_REQUIRED_RESULT Protocol::ChangeNotificationPtr notification() const;
    void setNotification(const Protocol::ChangeNotificationPtr &ntf);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

#endif
