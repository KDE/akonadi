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

#include "contactgrouplineedit_p.h"

#include "contactcompletionmodel_p.h"

#include <akonadi/entitytreemodel.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <klocale.h>

#include <QtCore/QAbstractItemModel>
#include <QtGui/QAction>
#include <QtGui/QCompleter>
#include <QtGui/QMenu>

ContactGroupLineEdit::ContactGroupLineEdit( QWidget *parent )
  : QLineEdit( parent ),
    mCompleter( 0 ),
    mContainsReference( false )
{
}

void ContactGroupLineEdit::setCompletionModel( QAbstractItemModel *model )
{
  mCompleter = new QCompleter( model, this );
  mCompleter->setCompletionColumn( Akonadi::ContactCompletionModel::NameAndEmailColumn );
  connect( mCompleter, SIGNAL( activated( const QModelIndex& ) ),
           this, SLOT( autoCompleted( const QModelIndex& ) ) );

  setCompleter( mCompleter );
}

bool ContactGroupLineEdit::containsReference() const
{
  return mContainsReference;
}

void ContactGroupLineEdit::setContactData( const KABC::ContactGroup::Data &data )
{
  mContactData = data;
  mContainsReference = false;

  setText( QString::fromLatin1( "%1 <%2>" ).arg( data.name() ).arg( data.email() ) );
}

KABC::ContactGroup::Data ContactGroupLineEdit::contactData() const
{
  QString fullName, email;
  KABC::Addressee::parseEmailAddress( text(), fullName, email );

  if ( fullName.isEmpty() || email.isEmpty() )
    return KABC::ContactGroup::Data();

  KABC::ContactGroup::Data data( mContactData );
  data.setName( fullName );
  data.setEmail( email );

  return data;
}

void ContactGroupLineEdit::setContactReference( const KABC::ContactGroup::ContactReference &reference )
{
  mContactReference = reference;
  mContainsReference = true;

  updateView( reference.uid(), reference.preferredEmail() );
}

KABC::ContactGroup::ContactReference ContactGroupLineEdit::contactReference() const
{
  return mContactReference;
}

void ContactGroupLineEdit::autoCompleted( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Akonadi::Item item = index.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
  if ( !item.isValid() )
    return;

  mContainsReference = true;

  updateView( item );
}

void ContactGroupLineEdit::updateView( const QString &uid, const QString &preferredEmail )
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Akonadi::Item( uid.toLongLong() ) );
  job->fetchScope().fetchFullPayload();
  job->setProperty( "preferredEmail", preferredEmail );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchDone( KJob* ) ) );
}

void ContactGroupLineEdit::fetchDone( KJob *job )
{
  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );

  if ( !fetchJob->items().isEmpty() ) {
    const Akonadi::Item item = fetchJob->items().first();
    updateView( item, fetchJob->property( "preferredEmail" ).toString() );
  }
}

void ContactGroupLineEdit::updateView( const Akonadi::Item &item, const QString &preferredEmail )
{
  if ( !item.hasPayload<KABC::Addressee>() )
    return;

  const KABC::Addressee contact = item.payload<KABC::Addressee>();

  QString email( preferredEmail );
  if ( email.isEmpty() )
    email = requestPreferredEmail( contact );

  QString name = contact.formattedName();
  if ( name.isEmpty() )
    name = contact.assembledName();

  if ( email.isEmpty() )
    setText( QString::fromLatin1( "%1" ).arg( name ) );
  else
    setText( QString::fromLatin1( "%1 <%2>" ).arg( name ).arg( email ) );

  mContactReference.setUid( QString::number( item.id() ) );

  if ( contact.preferredEmail() != email )
    mContactReference.setPreferredEmail( email );
}

QString ContactGroupLineEdit::requestPreferredEmail( const KABC::Addressee &contact ) const
{
  const QStringList emails = contact.emails();

  if ( emails.isEmpty() ) {
    qDebug( "ContactGroupLineEdit::requestPreferredEmail(): Warning!!! no email addresses available" );
    return QString();
  }

  if ( emails.count() == 1 )
    return emails.first();

  QAction *action = 0;

  QMenu menu;
  menu.setTitle( i18n( "Select preferred email address" ) );
  for ( int i = 0; i < emails.count(); ++i ) {
    action = menu.addAction( emails.at( i ) );
    action->setData( i );
  }

  action = menu.exec( mapToGlobal( QPoint( x() + width()/2, y() + height()/2 ) ) );
  if ( !action )
    return emails.first(); // use preferred email

  return emails.at( action->data().toInt() );
}

#include "contactgrouplineedit_p.moc"
