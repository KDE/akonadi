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

#include "contactviewerdialog.h"

#include "contactviewer.h"

#include <akonadi/item.h>
#include <klocale.h>

#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class ContactViewerDialog::Private
{
  public:
    ContactViewer *mViewer;
};

ContactViewerDialog::ContactViewerDialog( QWidget *parent )
  : KDialog( parent ), d( new Private )
{
  setCaption( i18n( "Show Contact" ) );
  setButtons( Ok );

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QVBoxLayout *layout = new QVBoxLayout( mainWidget );

  d->mViewer = new ContactViewer;
  layout->addWidget( d->mViewer );

  // forward the signals from ContactViewer
  connect( d->mViewer, SIGNAL( urlClicked( const QUrl& ) ),
           this, SIGNAL( urlClicked( const QUrl& ) ) );
  connect( d->mViewer, SIGNAL( emailClicked( const QString&, const QString& ) ),
           this, SIGNAL( emailClicked( const QString&, const QString& ) ) );
  connect( d->mViewer, SIGNAL( phoneNumberClicked( const KABC::PhoneNumber& ) ),
           this, SIGNAL( phoneNumberClicked( const KABC::PhoneNumber& ) ) );
  connect( d->mViewer, SIGNAL( addressClicked( const KABC::Address& ) ),
           this, SIGNAL( addressClicked( const KABC::Address& ) ) );

  setInitialSize( QSize( 500, 600 ) );
}

ContactViewerDialog::~ContactViewerDialog()
{
  delete d;
}

Akonadi::Item ContactViewerDialog::contact() const
{
  return d->mViewer->contact();
}

void ContactViewerDialog::setContact( const Akonadi::Item &contact )
{
  d->mViewer->setContact( contact );
}

#include "contactviewerdialog.moc"
