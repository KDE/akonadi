/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_CHANGENOTIFICATION_H
#define AKONADI_CHANGENOTIFICATION_H

#include <QDateTime>
#include <QVector>
#include <QSharedDataPointer>
#include <QSharedPointer>

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
    enum Type {
        Items,
        Collection,
        Tag,
        Relation,
        Subscription
    };

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
