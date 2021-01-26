/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ACTIONSTATEMANAGER_P_H
#define AKONADI_ACTIONSTATEMANAGER_P_H

#include "collection.h"
#include "item.h"

class QObject;

namespace Akonadi
{
/**
 * @short A helper class to manage action states.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ActionStateManager
{
public:
    /*
     * Creates a new action state manager.
     */
    explicit ActionStateManager() = default;

    virtual ~ActionStateManager() = default;

    /**
     * Updates the states according to the selected collections and items.
     * @param collections selected collections (from the folder tree)
     * @param favoriteCollections selected collections (among the ones marked as favorites)
     * @param items selected items
     */
    void updateState(const Collection::List &collections, const Collection::List &favoriteCollections, const Item::List &items);

    /**
     * Sets the @p receiver object that will actually update the states.
     *
     * The object must provide the following three slots:
     *   - void enableAction( int, bool )
     *   - void updatePluralLabel( int, int )
     *   - bool isFavoriteCollection( const Akonadi::Collection& )
     * @param receiver object that will actually update the states.
     */
    void setReceiver(QObject *receiver);

protected:
    virtual bool isRootCollection(const Collection &collection) const;
    virtual bool isResourceCollection(const Collection &collection) const;
    virtual bool isFolderCollection(const Collection &collection) const;
    virtual bool isSpecialCollection(const Collection &collection) const;
    virtual bool isFavoriteCollection(const Collection &collection) const;
    virtual bool hasResourceCapability(const Collection &collection, const QString &capability) const;
    virtual bool collectionCanHaveItems(const Collection &collection) const;

    virtual void enableAction(int action, bool state);
    virtual void updatePluralLabel(int action, int count);
    virtual void updateAlternatingAction(int action);

private:
    Q_DISABLE_COPY_MOVE(ActionStateManager)

    QObject *mReceiver = nullptr;
};

}

#endif
