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

#include "fetchjobcalendar.h"
#include "etmcalendar.h"
#include <kcalcore/incidence.h>

#include <QString>
#include <QWidget>

namespace Akonadi {

class GroupwareUiDelegate
{
  public:
    virtual ~GroupwareUiDelegate();
    virtual void requestIncidenceEditor( const Akonadi::Item &item ) = 0;

    virtual void setCalendar( const Akonadi::ETMCalendar::Ptr &calendar ) = 0;
    virtual void createCalendar() = 0;
};
  
class InvitationHandler : public QObject
{
  Q_OBJECT
public:
  enum Result {
    ResultError,      /**< An unexpected error occured */
    ResultSuccess     /**< The invitation was successfuly handled. */
  };
  explicit InvitationHandler( const Akonadi::FetchJobCalendar::Ptr &, QObject *parent = 0 );
  ~InvitationHandler();
    
  void handleInvitation( const QString &receiver, const QString &iCal, const QString &type );

Q_SIGNALS:
  void finished( Akonadi::InvitationHandler::Result result, const QString &errorMessage );

    /**
      This signal is emitted when an invitation for a counter proposal is sent.
      @param incidence The incidence for which the counter proposal must be specified.
     */ //TODO_SERGIO: connect this
    void editorRequested( const KCalCore::Incidence::Ptr &incidence );

private:
  Q_DISABLE_COPY( InvitationHandler )
  class Private;
  Private *const d;
};
  
}

#endif
