/*
  Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef AKONADI_CALENDAR_SCHEDULER_P_H
#define AKONADI_CALENDAR_SCHEDULER_P_H

#include "calendarbase.h"

#include <kcalcore/schedulemessage.h>
#include <kcalcore/incidencebase.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

namespace KCalCore {
class ICalFormat;
class FreeBusyCache;
}

namespace Akonadi {
/**
  This class provides an encapsulation of iTIP transactions (RFC 2446).
  It is an abstract base class for inheritance by implementations of the
  iTIP scheme like iMIP or iRIP.
*/
class Scheduler : public QObject
{
    Q_OBJECT
public:
    enum Result {
        ResultSuccess,
        ResultAssigningDifferentTypes,
        ResultOutatedUpdate,
        ResultErrorDelete,
        ResultIncidenceToDeleteNotFound,
        ResultGenericError,
        ResultNoFreeBusyCache,
        ResultErrorSavingFreeBusy,
        ResultCreatingError,
        ResultModifyingError,
        ResultDeletingError,
        ResultUnsupported,
        ResultUserCancelled
    };

    /**
      Creates a scheduler for calendar specified as argument.
    */
    explicit Scheduler(QObject *parent = 0);
    ~Scheduler();

    void setShowDialogs(bool enable);

    /**
      * Notify @p recipients about @p incidence
      *
      * @param incidence the incidence to send
      * @param recipients the people to send it to
    */
    virtual void publish(const KCalCore::IncidenceBase::Ptr &incidence,
                         const QString &recipients) = 0;
    /**
      Performs iTIP transaction on incidence. The method is specified as the
      method argument and can be any valid iTIP method.

      @param incidence the incidence for the transaction. Must be valid.
      @param method the iTIP transaction method to use.
    */
    virtual void performTransaction(const KCalCore::IncidenceBase::Ptr &incidence,
                                    KCalCore::iTIPMethod method) = 0;

    /**
      Performs iTIP transaction on incidence to specified recipient(s).
      The method is specified as the method argumanet and can be any valid iTIP method.

      @param incidence the incidence for the transaction. Must be valid.
      @param method the iTIP transaction method to use.
      @param recipients the receipients of the transaction.
    */
    virtual void performTransaction(const KCalCore::IncidenceBase::Ptr &incidence,
                                    KCalCore::iTIPMethod method,
                                    const QString &recipients) = 0;

    /**
      Accepts the transaction. The incidence argument specifies the iCal
      component on which the transaction acts. The status is the result of
      processing a iTIP message with the current calendar and specifies the
      action to be taken for this incidence.

      @param incidence the incidence for the transaction. Must be valid.
      @param calendar a loaded calendar.
      @param method iTIP transaction method to check.
      @param status scheduling status.
      @param email the email address of the person for whom this
      transaction is to be performed.

      Listen to the acceptTransactionFinished() signal to know the success.
    */
    void acceptTransaction(const KCalCore::IncidenceBase::Ptr &incidence,
                           const Akonadi::CalendarBase::Ptr &calendar,
                           KCalCore::iTIPMethod method,
                           KCalCore::ScheduleMessage::Status status,
                           const QString &email = QString());

    /**
      Returns the directory where the free-busy information is stored.
    */
    virtual QString freeBusyDir() const = 0;

    /**
      Sets the free/busy cache used to store free/busy information.
    */
    void setFreeBusyCache(KCalCore::FreeBusyCache *);

    /**
      Returns the free/busy cache.
    */
    KCalCore::FreeBusyCache *freeBusyCache() const;

protected:
    void acceptPublish(const KCalCore::IncidenceBase::Ptr &,
                       const Akonadi::CalendarBase::Ptr &calendar,
                       KCalCore::ScheduleMessage::Status status,
                       KCalCore::iTIPMethod method);

    void acceptRequest(const KCalCore::IncidenceBase::Ptr &,
                       const Akonadi::CalendarBase::Ptr &calendar,
                       KCalCore::ScheduleMessage::Status status,
                       const QString &email);

    void acceptAdd(const KCalCore::IncidenceBase::Ptr &,
                   KCalCore::ScheduleMessage::Status status);

    void acceptCancel(const KCalCore::IncidenceBase::Ptr &,
                      const Akonadi::CalendarBase::Ptr &calendar,
                      KCalCore::ScheduleMessage::Status status,
                      const QString &attendee);

    void acceptDeclineCounter(const KCalCore::IncidenceBase::Ptr &,
                              KCalCore::ScheduleMessage::Status status);

    void acceptReply(const KCalCore::IncidenceBase::Ptr &,
                     const Akonadi::CalendarBase::Ptr &calendar,
                     KCalCore::ScheduleMessage::Status status,
                     KCalCore::iTIPMethod method);

    void acceptRefresh(const KCalCore::IncidenceBase::Ptr &,
                       KCalCore::ScheduleMessage::Status status);

    void acceptCounter(const KCalCore::IncidenceBase::Ptr &,
                       KCalCore::ScheduleMessage::Status status);

    void acceptFreeBusy(const KCalCore::IncidenceBase::Ptr &, KCalCore::iTIPMethod method);
    KCalCore::ICalFormat *mFormat;

Q_SIGNALS:
    void transactionFinished(Akonadi::Scheduler::Result, const QString &errorMessage);
private Q_SLOTS:
    void handleCreateFinished(bool success, const QString &errorMessage);
    void handleModifyFinished(bool success, const QString &errorMessage);
    void handleDeleteFinished(bool success, const QString &errorMessage);

private:
    void connectCalendar(const Akonadi::CalendarBase::Ptr &calendar);
    Q_DISABLE_COPY(Scheduler)
    struct Private;
    Private *const d;
};

}

#endif
