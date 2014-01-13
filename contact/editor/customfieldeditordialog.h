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

#ifndef CUSTOMFIELDEDITORDIALOG_H
#define CUSTOMFIELDEDITORDIALOG_H

#include <kdialog.h>

#include "customfieldsmodel.h"

class KComboBox;
class KLineEdit;
class QCheckBox;

class CustomFieldEditorDialog : public KDialog
{
    Q_OBJECT

public:
    explicit CustomFieldEditorDialog(QWidget *parent = 0);

    void setCustomField(const CustomField &field);
    CustomField customField() const;

private:
    KLineEdit *mTitle;
    KComboBox *mType;
    QCheckBox *mScope;
    KLineEdit *mKey;
    CustomField mCustomField;
};

#endif
