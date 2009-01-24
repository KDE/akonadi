/*
    This file is part of KContactManager.

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
  mIMEdit->setText( contact.custom( "KADDRESSBOOK", "X-IMAddress" ) );
}

void IMEditWidget::storeContact( KABC::Addressee &contact ) const
{
  if ( !mIMEdit->text().isEmpty() )
    contact.insertCustom( "KADDRESSBOOK", "X-IMAddress", mIMEdit->text() );
  else
    contact.removeCustom( "KADDRESSBOOK", "X-IMAddress" );
}

void IMEditWidget::setReadOnly( bool readOnly )
{
  mIMEdit->setReadOnly( readOnly );
}

#include "imeditwidget.moc"
