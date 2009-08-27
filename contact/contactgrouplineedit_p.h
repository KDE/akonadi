/*
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

#ifndef CONTACTLINEEDIT_H
#define CONTACTLINEEDIT_H

#include <QtGui/QLineEdit>

#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

class QAbstractItemModel;
class QCompleter;
class QModelIndex;

class KJob;

class ContactGroupLineEdit : public QLineEdit
{
  Q_OBJECT

  public:
    ContactGroupLineEdit( QWidget *parent = 0 );

    void setCompletionModel( QAbstractItemModel *model );

    bool containsReference() const;

    void setContactData( const KABC::ContactGroup::Data &data );

    KABC::ContactGroup::Data contactData() const;

    void setContactReference( const KABC::ContactGroup::ContactReference &reference );

    KABC::ContactGroup::ContactReference contactReference() const;

  private Q_SLOTS:
    void autoCompleted( const QModelIndex& );
    void fetchDone( KJob* );

  private:
    void updateView( const QString &uid, const QString &preferredEmail = QString() );
    void updateView( const Akonadi::Item &item, const QString &preferredEmail = QString() );
    QString requestPreferredEmail( const KABC::Addressee &contact ) const;

    QCompleter *mCompleter;
    bool mContainsReference;
    KABC::ContactGroup::Data mContactData;
    KABC::ContactGroup::ContactReference mContactReference;
};

#endif
