/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef NAMEEDITWIDGET_H
#define NAMEEDITWIDGET_H

#include <QWidget>

#include <kabc/addressee.h>

class QLineEdit;
class QToolButton;
/**
 * @short A widget for editing the name of a contact.
 *
 * The widget will show the name in a lineedit
 * and provides a button to open a dialog for editing of
 * the single name components.
 */
class NameEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NameEditWidget(QWidget *parent = 0);
    ~NameEditWidget();

    /**
     * @param contact KABC contact's addressee to load
     */
    void loadContact(const KABC::Addressee &contact);
    /**
     * @param contact KABC contact's addressee to store
     */
    void storeContact(KABC::Addressee &contact) const;
    /**
     * @param readOnly sets readonly mode
     */
    void setReadOnly(bool readOnly);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the name has been changed.
     *
     * @param contact A dummy contact that contains only the name components.
     */
    void nameChanged(const KABC::Addressee &contact);

private Q_SLOTS:
    void textChanged(const QString &text);
    void openNameEditDialog();

private:
    QLineEdit *mNameEdit;
    KABC::Addressee mContact;
    QToolButton *mButtonEdit;
};

#endif
