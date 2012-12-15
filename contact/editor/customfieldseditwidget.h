/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef CUSTOMFIELDSEDITWIDGET_H
#define CUSTOMFIELDSEDITWIDGET_H

#include <QWidget>

#include "customfieldsmodel.h"

namespace KABC {
class Addressee;
}

class QPushButton;
class QTreeView;

void splitCustomField(  const QString &str, QString &app, QString &name, QString &value );

class CustomFieldsEditWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit CustomFieldsEditWidget( QWidget *parent = 0 );
    ~CustomFieldsEditWidget();

    void loadContact( const KABC::Addressee &contact );
    void storeContact( KABC::Addressee &contact ) const;

    void setReadOnly( bool readOnly );

    void setLocalCustomFieldDescriptions( const QVariantList &descriptions );
    QVariantList localCustomFieldDescriptions() const;

  private Q_SLOTS:
    void slotAdd();
    void slotEdit();
    void slotRemove();
    void slotUpdateButtons();

  private:
    QTreeView *mView;

    QPushButton *mAddButton;
    QPushButton *mEditButton;
    QPushButton *mRemoveButton;

    bool mReadOnly;
    CustomFieldsModel *mModel;
    CustomField::List mLocalCustomFields;
};

#endif
