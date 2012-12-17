/*
  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

  Copyright (C) 2010 Bertjan Broeksema <broeksema@kde.org>
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef _AKONADI_CALENDAR_INVITATION_HANDLER_H
#define _AKONADI_CALENDAR_INVITATION_HANDLER_H

#include "akonadi-calendar_export.h"
#include "fetchjobcalendar.h"
#include "etmcalendar.h"

#include <kcalcore/incidence.h>
#include <kcalcore/schedulemessage.h>

#include <QString>
#include <QWidget>

//TODO rename this class to reflect it's functionality
//TODO documentation

namespace Akonadi {

/**
 * @short Ui delegate for editing counter proposals.
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT GroupwareUiDelegate
{
  public:
    virtual ~GroupwareUiDelegate();
    virtual void requestIncidenceEditor( const Akonadi::Item &item ) = 0;

    virtual void setCalendar( const Akonadi::ETMCalendar::Ptr &calendar ) = 0;
    virtual void createCalendar() = 0;
};

/**
 * @short Handles sending of iTip invitations aswell as processing incoming ones.
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT InvitationHandler : public QObject
{
  Q_OBJECT
public:
  enum Result {
    ResultError,      /**< An unexpected error occured */
    ResultSuccess     /**< The invitation was successfuly handled. */
  };

  /**
   * Creates a new InvitationHandler instance.
   */
  explicit InvitationHandler( QObject *parent = 0 );

  /**
   * Destroys this instance.
   */
  ~InvitationHandler();

  /**
   * Processes a received iTip message.
   *
   * @param receiver
   * @param iCal
   * @param type
   *
   * @see iTipMessageProcessed()
   */
  void processiTIPMessage( const QString &receiver, const QString &iCal, const QString &type );

  /**
   * Sends an iTip message.
   *
   * @param method iTip method
   * @param incidence Incidence for which we're sending the iTip message.
   *                  Should contain a list of attendees.
   * @param parentWidget 
   */
  void sendiTIPMessage( KCalCore::iTIPMethod method,
                        const KCalCore::Incidence::Ptr &incidence,
                        QWidget *parentWidget = 0 );

  void publishInformation( const KCalCore::Incidence::Ptr &incidence, QWidget *parentWidget = 0 );

  void sendAsICalendar( const KCalCore::Incidence::Ptr &incidence, QWidget *parentWidget = 0 );


  /**
   * Sets the delegate to edit counter proposals.
   */
  void setGroupwareUiDelegate( GroupwareUiDelegate * );

Q_SIGNALS:
  /**
   * Sent after processing an incoming iTip message.
   *
   * @param result success of the operation.
   * @param errorMessage translated error message suitable for user dialogs.
   *                     Empty if the operation was successul
   */
  void iTipMessageProcessed( Akonadi::InvitationHandler::Result result,
                             const QString &errorMessage );

  /**
   * Signal emitted after an iTip message was sent through sendiTIPMessage().
   */
  void iTipMessageSent( Akonadi::InvitationHandler::Result, const QString &errorMessage );

  void informationPublished( Akonadi::InvitationHandler::Result, const QString &errorMessage );

  void sentAsICalendar( Akonadi::InvitationHandler::Result, const QString &errorMessage );

private:
  Q_DISABLE_COPY( InvitationHandler )
  class Private;
  Private *const d;
};
  
}

#endif
