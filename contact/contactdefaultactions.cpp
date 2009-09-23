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

#include "contactdefaultactions.h"

#include "actions/dialphonenumberaction.h"
#include "actions/showaddressaction.h"

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <ktoolinvocation.h>

#include <QtCore/QUrl>

using namespace Akonadi;

ContactDefaultActions::ContactDefaultActions( QObject *parent )
  : QObject( parent ), d( 0 )
{
}

ContactDefaultActions::~ContactDefaultActions()
{
}

void ContactDefaultActions::connectToView( QObject *view )
{
  const QMetaObject *metaObject = view->metaObject();

  if ( metaObject->indexOfSignal( QMetaObject::normalizedSignature( "urlClicked( const QUrl& )" ) ) != -1 )
    connect( view, SIGNAL( urlClicked( const QUrl& ) ), SLOT( showUrl( const QUrl& ) ) );

  if ( metaObject->indexOfSignal( QMetaObject::normalizedSignature( "emailClicked( const QString&, const QString& )" ) ) != -1 )
    connect( view, SIGNAL( emailClicked( const QString&, const QString& ) ),
             this, SLOT( sendEmail( const QString&, const QString& ) ) );

  if ( metaObject->indexOfSignal( QMetaObject::normalizedSignature( "phoneNumberClicked( const KABC::PhoneNumber& )" ) ) != -1 )
    connect( view, SIGNAL( phoneNumberClicked( const KABC::PhoneNumber& ) ),
             this, SLOT( dialPhoneNumber( const KABC::PhoneNumber& ) ) );

  if ( metaObject->indexOfSignal( QMetaObject::normalizedSignature( "addressClicked( const KABC::Address& )" ) ) != -1 )
    connect( view, SIGNAL( addressClicked( const KABC::Address& ) ),
             this, SLOT( showAddress( const KABC::Address& ) ) );
}

void ContactDefaultActions::showUrl( const QUrl &url )
{
  KToolInvocation::invokeBrowser( url.toString() );
}

void ContactDefaultActions::sendEmail( const QString &name, const QString &address )
{
  KABC::Addressee contact;
  contact.setNameFromString( name );

  KUrl mailtoUrl( QString::fromLatin1( "mailto:%1" ).arg( contact.fullEmail( address ) ) );

  KToolInvocation::invokeMailer( mailtoUrl );
}

void ContactDefaultActions::dialPhoneNumber( const KABC::PhoneNumber &number )
{
  DialPhoneNumberAction action;
  action.dialNumber( number );
}

void ContactDefaultActions::showAddress( const KABC::Address &address )
{
  ShowAddressAction action;
  action.showAddress( address );
}

#include "contactdefaultactions.moc"
