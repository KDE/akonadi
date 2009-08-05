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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "nameeditwidget.h"

#include <QtCore/QString>
#include <QtGui/QHBoxLayout>

#include <kabc/addressee.h>
#include <kdialog.h>
#include <klineedit.h>

NameEditWidget::NameEditWidget( QWidget *parent )
  : QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( KDialog::spacingHint() );

  mNameEdit = new KLineEdit;
  layout->addWidget( mNameEdit );

  connect( mNameEdit, SIGNAL( textChanged( const QString& ) ), SLOT( textChanged( const QString& ) ) );
}

NameEditWidget::~NameEditWidget()
{
}

void NameEditWidget::setReadOnly( bool readOnly )
{
  mNameEdit->setReadOnly( readOnly );
}

void NameEditWidget::loadContact( const KABC::Addressee &contact )
{
  mNameEdit->setText( contact.assembledName() );
}

void NameEditWidget::storeContact( KABC::Addressee &contact ) const
{
  contact.setNameFromString( mNameEdit->text() );
}

void NameEditWidget::textChanged( const QString &text )
{
  KABC::Addressee contact;
  contact.setNameFromString( text );

  emit nameChanged( contact );
}

#include "nameeditwidget.moc"
