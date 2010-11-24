/*
 *  Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *  Copyright (c) 2009 - 2010 Tobias Koenig <tokoe@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef AKONADI_STANDARDCALENDARACTIONMANAGER_H
#define AKONADI_STANDARDCALENDARACTIONMANAGER_H

#include "akonadi-calendar_export.h"

#include <akonadi/standardactionmanager.h>

#include <QtCore/QObject>

//needed for windows ce, its defined somewhere
#undef CreateEvent

class KAction;
class KActionCollection;
class QItemSelectionModel;
class QWidget;

namespace Akonadi
{

class Item;
/**
 * @short Manages calendar specific actions for collection and item views.
 *
 * @author Casey Link <unnamedrambler@gmail.com>
 * @since 4.6
 */
class AKONADI_CALENDAR_EXPORT StandardCalendarActionManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Describes the supported actions.
     */
    enum Type {
      CreateEvent = StandardActionManager::LastType + 1, ///< Creates a new event
      CreateTodo,                                        ///< Creates a new todo
      CreateSubTodo,                                     ///< Creates a new sub-todo
      CreateJournal,                                     ///< Creates a new journal
      EditIncidence,                                     ///< Edit currently selected event/todo/journal
      LastType                                           ///< Marks last action
    };

    /**
     * Creates a new standard calendar action manager.
     *
     * @param actionCollection The action collection to operate on.
     * @param parent The parent widget.
     */
    explicit StandardCalendarActionManager( KActionCollection *actionCollection, QWidget *parent = 0 );

    /**
     * Destroys the standard contact action manager.
     */
    ~StandardCalendarActionManager();

    /**
     * Sets the collection selection model based on which the collection
     * related actions should operate. If none is set, all collection actions
     * will be disabled.
     */
    void setCollectionSelectionModel( QItemSelectionModel *selectionModel );

    /**
     * Sets the item selection model based on which the item related actions
     * should operate. If none is set, all item actions will be disabled.
     */
    void setItemSelectionModel( QItemSelectionModel* selectionModel );

    /**
     * Creates the action of the given type and adds it to the action collection
     * specified in the constructor if it does not exist yet. The action is
     * connected to its default implementation provided by this class.
     */
    KAction* createAction( Type type );

    /**
     * Creates the action of the given type and adds it to the action collection
     * specified in the constructor if it does not exist yet. The action is
     * connected to its default implementation provided by this class.
     */
    KAction* createAction( StandardActionManager::Type type );

    /**
     * Convenience method to create all standard actions.
     * @see createAction()
     */
    void createAllActions();

    /**
     * Returns the action of the given type, 0 if it has not been created (yet).
     */
    KAction* action( Type type ) const;

    /**
     * Returns the action of the given type, 0 if it has not been created (yet).
     */
    KAction* action( StandardActionManager::Type type ) const;

    /**
     * Sets the label of the action @p type to @p text, which is used during
     * updating the action state and substituted according to the number of
     * selected objects. This is mainly useful to customize the label of actions
     * that can operate on multiple objects.
     *
     * Example:
     * @code
     * acctMgr->setActionText( Akonadi::StandardActionManager::CopyItems,
     *                         ki18np( "Copy Item", "Copy %1 Items" ) );
     * @endcode
     */
    void setActionText( StandardActionManager::Type type, const KLocalizedString &text );

    /**
     * Sets whether the default implementation for the given action @p type
     * shall be executed when the action is triggered.
     *
     * @param intercept If @c false, the default implementation will be executed,
     *                  if @c true no action is taken.
     */
    void interceptAction( Type type, bool intercept = true );

    /**
     * Sets whether the default implementation for the given action @p type
     * shall be executed when the action is triggered.
     *
     * @param intercept If @c false, the default implementation will be executed,
     *                  if @c true no action is taken.
     */
    void interceptAction( StandardActionManager::Type type, bool intercept = true );

    /**
     * Returns the list of collections that are currently selected.
     * The list is empty if no collection is currently selected.
     */
    Akonadi::Collection::List selectedCollections() const;

    /**
     * Returns the list of items that are currently selected.
     * The list is empty if no item is currently selected.
     */
    Akonadi::Item::List selectedItems() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the action state has been updated.
     * In case you have special needs for changing the state of some actions,
     * connect to this signal and adjust the action state.
     */
    void actionStateUpdated();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotCreateEvent() )
    Q_PRIVATE_SLOT( d, void slotCreateTodo() )
    Q_PRIVATE_SLOT( d, void slotCreateSubTodo() )
    Q_PRIVATE_SLOT( d, void slotCreateJournal() )
    Q_PRIVATE_SLOT( d, void slotEditIncidence() )
    Q_PRIVATE_SLOT( d, void updateActions() )
    //@endcond
};

}

#endif
