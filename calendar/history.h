/*
  Copyright (C) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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


#ifndef AKONADI_HISTORY_H
#define AKONADI_HISTORY_H

#include "akonadi-calendar_export.h"
#include "incidencechanger.h"

#include <KCalCore/Incidence>

#include <Akonadi/Item>

#include <QWidget>

namespace Akonadi {

class IncidenceChanger;

/**
   @short History class for implementing undo/redo in your application.

   Keeps a stack of all changes ( incidence adds, edits and deletes ) and
   will fire Item{Modify|Add|Delete}Jobs when undo() and redo() slots are
   called.

   Doesn't really use Item*Jobs directly, it uses IncidenceChanger, so invitation
   e-mails are sent.

   TODO: Talk about atomic operations.

   @code
      TODO:
   @endcode

   @author Sérgio Martins <iamsergio@gmail.com>
   @since
*/

class AKONADI_CALENDAR_EXPORT History : public QObject {
  Q_OBJECT
  public:

    /**
     * This enum describes the possible result codes (success/error values)
     * for an undo or redo operation.
     * @see undone()
     * @see redone()
     */
    enum ResultCode {
      ResultCodeSuccess = 0, ///< Success
      ResultCodeError, ///< An error occurred. Call lastErrorString() for the error message. This isn't very verbose because IncidenceChanger hasn't been refactored yet.
      ResultCodeIncidenceChangerError ///< IncidenceChanger returned false and didn't even create the job. This error is temporary. IncidenceChanger needs to be refactored.

    };

    /**
     * Destroys the History instance.
     */
    ~History();

    /**
     *  Pushes an incidence creation into the undo stack. The creation can be undone calling
     * undo().
     *
     * @param incidence the item that was created. Must be valid and have a Incidence::Ptr payload
     * @param atomicOperationId if not 0, specifies which group of changes this change belongs too.
     *        When a change is undone/redone, all other changes which are in the same group are
     *        undone/redone too
     */
    void recordCreation( const Akonadi::Item &item,
                         const QString &description,
                         const uint atomicOperationId = 0 );

    /**
     * Pushes an incidence modification into the undo stack. The modification can be undone calling
     * undo().
     *
     * @param oldItem item containing the payload before the change. Must be valid and contain an
     *        Incidence::Ptr payload
     * @param newitem item containing the new payload. Must be valid. Must be valid and contain an
     *        Incidence::Ptr payload
     * @param atomicOperationId if not 0, specifies which group of changes this change belongs too.
     *        When a change is undone/redone, all other changes which are in the same group are
     *        undone/redone too
     */
    void recordModification( const Akonadi::Item &oldItem,
                             const Akonadi::Item &newItem,
                             const QString &description,
                             const uint atomicOperationId = 0 );

    /**
     * Pushes an incidence deletion into the undo stack. The deletion can be undone calling
     * undo().
     *
     * @param item The item to delete. Must be valid.
     * @param atomicOperationId if not 0, specifies which group of changes this change belongs too.
     *        When a change is undone/redone, all other changes which are in the same group are
     *        undone/redone too
     */
    void recordDeletion( const Akonadi::Item &item,
                         const QString &description,
                         const uint atomicOperationId = 0 );

    /**
     * Pushes a list of incidence deletions into the undo stack. The deletions can be undone calling
     * undo() once.
     *
     * @param items The list of items to delete. All items must be valid.
     * @param atomicOperation If != 0, specifies which group of changes this change belongs too.
     *        Will be useful for atomic undoing/redoing, not implemented yet.
     */
    void recordDeletions( const Akonadi::Item::List &items,
                          const QString &description,
                          const uint atomicOperationId = 0 );

    /**
     * Returns the last error message.
     *
     * Call this immediately after catching the undone()/redone() signal
     * with an ResultCode != ResultCodeSuccess.
     *
     * The message is translated.
     */
    QString lastErrorString() const;

    /**
     * Reverts the change that's ontop of the undo stack.
     * Can't be called if there's an undo/redo operation running, Q_ASSERTs.
     * Can be called if the stack is empty, in this case, nothing happens.
     * This function is async, listen to signal undone() to know when the operation finishes.
     *
     * @param parent will be passed to dialogs created by IncidenceChanger, for example
     *        those which ask if you want to send invitations.
     *
     * @see redo()
     * @see undone()
     */
    void undo( QWidget *parent = 0 );

    /**
     * Reverts the change that's ontop of the redo stack.
     * Can't be called if there's an undo/redo operation running, Q_ASSERTs.
     * Can be called if the stack is empty, in this case, nothing happens.
     * This function is async, listen to signal redone() to know when the operation finishes.
     *
     * @param parent will be passed to dialogs created by IncidenceChanger, for example
     *        those which ask if you want to send invitations.
     *
     * @see undo()
     * @see redone()
     */
    void redo( QWidget *parent = 0 );

    /**
     * Reverts every change in the undostack.
     *
     * @param parent will be passed to dialogs created by IncidenceChanger, for example
     *        those which ask if you want to send invitations.
     */
    void undoAll( QWidget *parent = 0 );

    /**
     * Returns the number of changes available to be undone.
     */
    int undoCount() const;

    /**
     * Returns the number of changes available to be redone.
     */
    int redoCount() const;

    QString descriptionOfNextUndo() const;
    QString descriptionOfNextRedo() const;

  public Q_SLOTS:
    /**
     * Clears the undo and redo stacks.
     * Won't do anything if there's a undo/redo job currently running.
     *
     * @return true if the stacks were cleared, false if there was a job running
     */
    bool clear();

  Q_SIGNALS:
    /**
     * This signal is emitted when an undo operation finishes.
     * @param resultCode History::ResultCodeSuccess on success.
     * @see lastErrorString()
     */
    void undone( Akonadi::History::ResultCode resultCode );

    /**
     * This signal is emitted when an redo operation finishes.
     * @param resultCode History::ResultCodeSuccess on success.
     * @see lastErrorString()
     */
    void redone( Akonadi::History::ResultCode resultCode );

  private:
    void setEnabled( bool enabled );

    /**
     * Creates an History instance.
     * Only IncidenceChanger can create an History instance.
     * @param changer a valid pointer to an IncidenceChanger. Ownership is not taken.
     */
    friend class IncidenceChanger;
    friend class Entry;
    explicit History( IncidenceChanger *changer );
    // Used by unit-tests
    Akonadi::IncidenceChanger* incidenceChanger() const;

    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
  };
}

#endif
