/*
  IM addresses editor widget for KAddressbook

  Copyright (c) 2004 Will Stephenson   <lists@stevello.free-online.co.uk>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef IMEDITORDIALOG_H
#define IMEDITORDIALOG_H

#include <kdialog.h>

#include "immodel.h"

class QPushButton;
class QTreeView;

class IMEditorDialog : public KDialog
{
  Q_OBJECT

  public:
    IMEditorDialog( QWidget *parent );
    ~IMEditorDialog() {}

    void setAddresses( const IMAddress::List &addresses );
    IMAddress::List addresses() const;

  private Q_SLOTS:
    void slotAdd();
    void slotRemove();
    void slotSetStandard();
    void slotUpdateButtons();

  private:
    QTreeView *mView;
    QPushButton *mAddButton;
    QPushButton *mRemoveButton;
    QPushButton *mStandardButton;

    IMModel *mModel;
};

#endif
