/*
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef AKONADI_PUBLISHDIALOG_H
#define AKONADI_PUBLISHDIALOG_H

#include "akonadi-calendar_export.h"

#include <kcalcore/attendee.h>
#include <KDE/KDialog>

class PublishDialog_base;

//TODO: documentation
// Uses akonadi-contact, so don't move this class to KCalUtils.
namespace Akonadi {

class AKONADI_CALENDAR_EXPORT PublishDialog : public KDialog
{
    Q_OBJECT
public:
    /**
     * Creates a new PublishDialog
     * @param parent the dialog's parent
     */
    explicit PublishDialog(QWidget *parent=0);

    /**
     * Destructor
     */
    ~PublishDialog();

    /**
     * Adds a new attendee to the dialog
     * @param attendee the attendee to add
     */
    void addAttendee(const KCalCore::Attendee::Ptr &attendee);

    /**
     * Returns a list of e-mail addresses.
     * //TODO: This should be a QStringList, but KCalUtils::Scheduler::publish() accepts a QString.
     */
    QString addresses() const;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
