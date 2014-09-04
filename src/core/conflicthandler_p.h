/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONFLICTHANDLER_P_H
#define AKONADI_CONFLICTHANDLER_P_H

#include <QtCore/QObject>

#include "item.h"

class KJob;

namespace Akonadi {

class Session;

/**
 * @short A class to handle conflicts in Akonadi
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ConflictHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * Describes the type of conflict that should be resolved by
     * the conflict handler.
     */
    enum ConflictType {
        LocalLocalConflict,   ///< Changes of two Akonadi client applications conflict.
        LocalRemoteConflict,  ///< Changes of an Akonadi client application and a resource conflict.
        BackendConflict       ///< Changes of a resource and the backend data conflict.
    };

    /**
     * Describes the strategy that should be used for resolving the conflict.
     */
    enum ResolveStrategy {
        UseLocalItem, ///< The local item overwrites the other item inside the Akonadi storage.
        UseOtherItem, ///< The local item is dropped and the other item from the Akonadi storage is used.
        UseBothItems  ///< Both items are kept in the Akonadi storage.
    };

    /**
     * Creates a new conflict handler.
     *
     * @param type The type of the conflict that should be resolved.
     * @param parent The parent object.
     */
    explicit ConflictHandler(ConflictType type, QObject *parent = 0);

    /**
     * Sets the items that causes the conflict.
     *
     * @param changedItem The item that has been changed, it needs the complete payload set.
     * @param conflictingItem The item from the Akonadi storage that is conflicting.
     *                        This needs only the id set, the payload will be refetched automatically.
     */
    void setConflictingItems(const Akonadi::Item &changedItem, const Akonadi::Item &conflictingItem);

public Q_SLOTS:
    /**
     * Starts the conflict handling.
     */
    void start();

Q_SIGNALS:
    /**
     * This signal is emitted whenever the conflict has been resolved
     * automatically or by the user.
     */
    void conflictResolved();

    /**
     * This signal is emitted whenever an error occurred during the conflict
     * handling.
     *
     * @param message A user visible string that describes the error.
     */
    void error(const QString &message);

private Q_SLOTS:
    void slotOtherItemFetched(KJob *);
    void slotUseLocalItemFinished(KJob *);
    void slotUseBothItemsFinished(KJob *);
    void resolve();

private:
    void useLocalItem();
    void useOtherItem();
    void useBothItems();

    ConflictType mConflictType;
    Akonadi::Item mChangedItem;
    Akonadi::Item mConflictingItem;

    Session *mSession;
};

}

#endif
