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

#ifndef _AKONADI_CALENDAR_ITIP_HANDLER_H
#define _AKONADI_CALENDAR_ITIP_HANDLER_H

#include "akonadi-calendar_export.h"
#include "etmcalendar.h"

#include <kcalcore/incidence.h>
#include <kcalcore/schedulemessage.h>

#include <QString>
#include <QWidget>

namespace MailTransport {
    class MessageQueueJob;
}

namespace KPIMIdentities {
    class Identity;
}

namespace Akonadi {

/**
 * @short Ui delegate for editing counter proposals.
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT GroupwareUiDelegate
{
public:
    virtual ~GroupwareUiDelegate();
    virtual void requestIncidenceEditor(const Akonadi::Item &item) = 0;

    virtual void setCalendar(const Akonadi::ETMCalendar::Ptr &calendar) = 0;
    virtual void createCalendar() = 0;
};

/**
 * @short Factory to create MailTransport::MessageQueueJob jobs.
 * @since 4.15
 */
class AKONADI_CALENDAR_EXPORT ITIPHandlerComponentFactory : public QObject
{
    Q_OBJECT
public:
    /*
     * Created a new ITIPHandlerComponentFactory object.
     */
    explicit ITIPHandlerComponentFactory(QObject *parent = 0);

    /*
     * deletes the object.
     */
    virtual ~ITIPHandlerComponentFactory();

    /*
     * @return A new MailTransport::MessageQueueJob object
     * @param incidence related to the mail
     * @param identity that is the mail sender
     * @param parent of the MailTransport::MessageQueueJob object
     */
    virtual MailTransport::MessageQueueJob* createMessageQueueJob(const KCalCore::IncidenceBase::Ptr &incidence, const KPIMIdentities::Identity &identity, QObject *parent = 0);
};

/**
 * @short Handles sending of iTip messages aswell as processing incoming ones.
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT ITIPHandler : public QObject
{
    Q_OBJECT
public:
    enum Result {
        ResultError,      /**< An unexpected error occurred */
        ResultSuccess,    /**< The invitation was successfuly handled. */
        ResultCancelled   /**< User cancelled the operation. @since 4.12 */
    };

    /**
     * Creates a new ITIPHandler instance.
     * creates a default ITIPHandlerComponentFactory object.
     */
    explicit ITIPHandler(QObject *parent = 0);

    /**
     * Create a new ITIPHandler instance.
     * @param factory is set to 0 a new factory is created.
     * @since 4.15
     */
    explicit ITIPHandler(ITIPHandlerComponentFactory *factory, QObject *parent);
    /**
     * Destroys this instance.
     */
    ~ITIPHandler();

    /**
     * Processes a received iTip message.
     *
     * @param receiver
     * @param iCal
     * @param type
     *
     * @see iTipMessageProcessed()
     */
    void processiTIPMessage(const QString &receiver, const QString &iCal, const QString &type);

    /**
     * Sends an iTip message.
     *
     * @param method iTip method
     * @param incidence Incidence for which we're sending the iTip message.
     *                  Should contain a list of attendees.
     * @param parentWidget
     */
    void sendiTIPMessage(KCalCore::iTIPMethod method,
                         const KCalCore::Incidence::Ptr &incidence,
                         QWidget *parentWidget = 0);

    /**
     * Publishes incidence @p incidence.
     * A publish dialog will prompt the user to input recipients.
     * @see rfc2446 3.2.1
     */
    void publishInformation(const KCalCore::Incidence::Ptr &incidence, QWidget *parentWidget = 0);

    /**
     * Sends an e-mail with the incidence attached as iCalendar source.
     * A dialog will prompt the user to input recipients.
     */
    void sendAsICalendar(const KCalCore::Incidence::Ptr &incidence, QWidget *parentWidget = 0);

    /**
     * Sets the UI delegate to edit counter proposals.
     */
    void setGroupwareUiDelegate(GroupwareUiDelegate *);

    /**
     * Sets the calendar that the itip handler should use.
     * The calendar should already be loaded.
     *
     * If none is set, a FetchJobCalendar will be created internally.
     */
    void setCalendar(const Akonadi::CalendarBase::Ptr &);

    /**
     * Sets if the ITIP handler should show dialogs on error.
     * Default is true, for compatibility reasons, but this will change in KDE5.
     * TODO_KDE5: use message delegates
     *
     * @since 4.12
     */
    void setShowDialogsOnError(bool enable);

    /**
     * Returns the calendar used by this itip handler.
     */
    Akonadi::CalendarBase::Ptr calendar() const;

Q_SIGNALS:
    /**
     * Sent after processing an incoming iTip message.
     *
     * @param result success of the operation.
     * @param errorMessage translated error message suitable for user dialogs.
     *                     Empty if the operation was successul
     */
    void iTipMessageProcessed(Akonadi::ITIPHandler::Result result,
                              const QString &errorMessage);

    /**
     * Signal emitted after an iTip message was sent through sendiTIPMessage()
     */
    void iTipMessageSent(Akonadi::ITIPHandler::Result, const QString &errorMessage);

    /**
     * Signal emitted after an incidence was published with publishInformation()
     */
    void informationPublished(Akonadi::ITIPHandler::Result, const QString &errorMessage);

    /**
     * Signal emitted after an incidence was sent with sendAsICalendar()
     */
    void sentAsICalendar(Akonadi::ITIPHandler::Result, const QString &errorMessage);

private:
    Q_DISABLE_COPY(ITIPHandler)
    class Private;
    Private *const d;
};

}

#endif
