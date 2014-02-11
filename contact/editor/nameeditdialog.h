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

#ifndef NAMEEDITDIALOG_H
#define NAMEEDITDIALOG_H

#include <kdialog.h>

class KLineEdit;
class KComboBox;

class NameEditDialog : public KDialog
{
public:
    explicit NameEditDialog(QWidget *parent = 0);

    void setFamilyName(const QString &name);
    QString familyName() const;

    void setGivenName(const QString &name);
    QString givenName() const;

    void setPrefix(const QString &prefix);
    QString prefix() const;

    void setSuffix(const QString &suffix);
    QString suffix() const;

    void setAdditionalName(const QString &name);
    QString additionalName() const;

private:
    KComboBox *mSuffixCombo;
    KComboBox *mPrefixCombo;
    KLineEdit *mFamilyNameEdit;
    KLineEdit *mGivenNameEdit;
    KLineEdit *mAdditionalNameEdit;
};

#endif
