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

#ifndef DISPLAYNAMEEDITWIDGET_H
#define DISPLAYNAMEEDITWIDGET_H

#include <QWidget>

#include <kabc/addressee.h>

class KComboBox;

/**
 * @short A widget for editing the display name of a contact.
 *
 * The widget will either use a predefined schema for formatting
 * the name or a custom one.
 */
class DisplayNameEditWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Describes what the display name should look like.
     */
    enum DisplayType {
        SimpleName,           ///< A name of the form: givenName familyName
        FullName,             ///< A name of the form: prefix givenName additionalName familyName suffix
        ReverseNameWithComma, ///< A name of the form: familyName, givenName
        ReverseName,          ///< A name of the form: familyName givenName
        Organization,         ///< The organization name
        CustomName            ///< Let the user input a display name
    };

    explicit DisplayNameEditWidget(QWidget *parent = 0);
    ~DisplayNameEditWidget();

    void loadContact(const KABC::Addressee &contact);
    void storeContact(KABC::Addressee &contact) const;

    void setReadOnly(bool readOnly);

    void setDisplayType(DisplayType type);
    DisplayType displayType() const;

public Q_SLOTS:
    void changeName(const KABC::Addressee &contact);
    void changeOrganization(const QString &organization);

protected:
    virtual bool eventFilter(QObject *object, QEvent *event);

private Q_SLOTS:
    void displayTypeChanged(int);
    void setComboBoxEditable(bool);

private:
    void updateView();

    KComboBox *mView;
    DisplayType mDisplayType;
    KABC::Addressee mContact;
    QWidget *mViewport;
    int mAdditionalPopupWidth;
};

#endif
