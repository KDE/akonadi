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

#include "ui_publishdialog_base.h"

#include <KCalCore/Attendee>
#include <KDialog>

class PublishDialog_base;

class PublishDialog : public KDialog
{
  Q_OBJECT
  public:
    explicit PublishDialog( QWidget *parent=0 );
    ~PublishDialog();

    void addAttendee( const KCalCore::Attendee::Ptr &attendee );
    QString addresses() const;

  signals:
    void numMessagesChanged( int );

  protected slots:
    void addItem();
    void removeItem();
    void openAddressbook();
    void updateItem();
    void updateInput();

  protected:
    Ui::PublishDialog_base mUI;
};

#endif
