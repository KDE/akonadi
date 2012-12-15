/*
IM address item editor widget for KDE PIM

Copyright 2012 Jonathan Marten <jjm@keelhaul.me.uk>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AKONADI_CONTACT_IMITEMDIALOG_H
#define AKONADI_CONTACT_IMITEMDIALOG_H

#include <kdialog.h>

#include "immodel.h"

class KComboBox;
class KLineEdit;

class IMItemDialog : public KDialog
{
  Q_OBJECT

  public:
    explicit IMItemDialog( QWidget *parent );
    ~IMItemDialog() {}

    void setAddress( const IMAddress &address );
    IMAddress address() const;

  private Q_SLOTS:
    void slotUpdateButtons();

  private:
    KComboBox *mProtocolCombo;
    KLineEdit *mNameEdit;
};

#endif
