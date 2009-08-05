/*
    This file is part of KAddressBook.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "freebusyeditwidget.h"

#include <QtGui/QHBoxLayout>

#include <kabc/addressee.h>
#include <kcal/freebusyurlstore.h>
#include <kurlrequester.h>

FreeBusyEditWidget::FreeBusyEditWidget( QWidget *parent )
  : QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );

  mURL = new KUrlRequester;
  layout->addWidget( mURL );
}

FreeBusyEditWidget::~FreeBusyEditWidget()
{
}

void FreeBusyEditWidget::loadContact( const KABC::Addressee &contact )
{
  if ( contact.preferredEmail().isEmpty() )
    return;

  mURL->setUrl( KCal::FreeBusyUrlStore::self()->readUrl( contact.preferredEmail() ) );
}

void FreeBusyEditWidget::storeContact( KABC::Addressee &contact ) const
{
  if ( contact.preferredEmail().isEmpty() )
    return;

  KCal::FreeBusyUrlStore::self()->writeUrl( contact.preferredEmail(), mURL->url().url() );
  KCal::FreeBusyUrlStore::self()->sync();
}

void FreeBusyEditWidget::setReadOnly( bool readOnly )
{
  mURL->setEnabled( !readOnly );
}

#include "freebusyeditwidget.moc"
