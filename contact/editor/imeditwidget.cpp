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

#include "imeditwidget.h"

#include <QtGui/QHBoxLayout>

#include <kabc/addressee.h>
#include <klineedit.h>

IMEditWidget::IMEditWidget( QWidget *parent )
  : QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );

  mIMEdit = new KLineEdit;
  layout->addWidget( mIMEdit );
}

IMEditWidget::~IMEditWidget()
{
}

void IMEditWidget::loadContact( const KABC::Addressee &contact )
{
  mIMEdit->setText( contact.custom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-IMAddress" ) ) );
}

void IMEditWidget::storeContact( KABC::Addressee &contact ) const
{
  if ( !mIMEdit->text().isEmpty() )
    contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-IMAddress" ), mIMEdit->text() );
  else
    contact.removeCustom( QLatin1String( "KADDRESSBOOK" ), QLatin1String( "X-IMAddress" ) );
}

void IMEditWidget::setReadOnly( bool readOnly )
{
  mIMEdit->setReadOnly( readOnly );
}

#include "imeditwidget.moc"
