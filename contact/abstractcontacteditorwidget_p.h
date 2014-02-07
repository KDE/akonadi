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

#ifndef AKONADI_ABSTRACTCONTACTEDITORWIDGET_P_H
#define AKONADI_ABSTRACTCONTACTEDITORWIDGET_P_H

#include <QWidget>

namespace KABC
{
class Addressee;
}

namespace Akonadi
{

class ContactMetaData;

class AbstractContactEditorWidget : public QWidget
{
public:
    /**
     * Creates a new abstract contact editor widget.
     *
     * @param parent The parent widget.
     */
    AbstractContactEditorWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
    }

    /**
     * Destroys the abstract contact editor widget.
     */
    ~AbstractContactEditorWidget() { }

    /**
     * @param contact loads the given contact into the editor widget
     */
    virtual void loadContact(const KABC::Addressee &contact, const Akonadi::ContactMetaData &metaData) = 0;

    /**
     * @param contact store the given contact into the editor widget
     */
    virtual void storeContact(KABC::Addressee &contact, Akonadi::ContactMetaData &metaData) const = 0;

    /**
     * @param readOnly set read-only mode
     */
    virtual void setReadOnly(bool readOnly) = 0;
};

}

#endif
