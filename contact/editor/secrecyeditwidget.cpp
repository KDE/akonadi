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

#include "secrecyeditwidget.h"

#include <QtGui/QVBoxLayout>

#include <kabc/addressee.h>
#include <kabc/secrecy.h>
#include <kcombobox.h>

SecrecyEditWidget::SecrecyEditWidget( QWidget *parent )
  : QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  mSecrecyCombo = new KComboBox( this );
  layout->addWidget( mSecrecyCombo );

  const KABC::Secrecy::TypeList list = KABC::Secrecy::typeList();
  KABC::Secrecy::TypeList::ConstIterator it;

  // (*it) is the type enum, which is also used as the index in the combo
  for ( it = list.begin(); it != list.end(); ++it )
    mSecrecyCombo->insertItem( *it, KABC::Secrecy::typeLabel( *it ) );
}

SecrecyEditWidget::~SecrecyEditWidget()
{
}

void SecrecyEditWidget::setReadOnly( bool readOnly )
{
  mSecrecyCombo->setEnabled( !readOnly );
}

void SecrecyEditWidget::loadContact( const KABC::Addressee &contact )
{
  if ( contact.secrecy().type() != KABC::Secrecy::Invalid )
    mSecrecyCombo->setCurrentIndex( contact.secrecy().type() );
}

void SecrecyEditWidget::storeContact( KABC::Addressee &contact ) const
{
  KABC::Secrecy secrecy;
  secrecy.setType( (KABC::Secrecy::Type)mSecrecyCombo->currentIndex() );

  contact.setSecrecy( secrecy );
}

#include "secrecyeditwidget.moc"
