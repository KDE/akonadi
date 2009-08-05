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

#include "dateeditwidget.h"

#include <kdatepicker.h>
#include <kglobal.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <libkdepim/kdatepickerpopup.h>

#include <QtGui/QContextMenuEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

DateLineEdit::DateLineEdit( QWidget *parent )
  : KLineEdit( parent )
{
  setReadOnly( true );
}

void DateLineEdit::contextMenuEvent( QContextMenuEvent *event )
{
  QMenu menu;
  menu.addAction( i18n( "Remove" ), this, SLOT( emitSignal() ) );

  menu.exec( event->globalPos() );
}

void DateLineEdit::emitSignal()
{
  emit resetDate();
}

DateEditWidget::DateEditWidget( QWidget *parent )
  : QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );

  mView = new DateLineEdit;
  layout->addWidget( mView );

  mButton = new QToolButton;
  mButton->setPopupMode(QToolButton::InstantPopup);
  mButton->setIcon( KIcon( "view-calendar-day" ) );
  layout->addWidget( mButton );

  mMenu = new KPIM::KDatePickerPopup( KPIM::KDatePickerPopup::DatePicker, QDate(), this );
  mButton->setMenu( mMenu );

  connect( mMenu, SIGNAL( dateChanged( const QDate& ) ), SLOT( dateSelected( const QDate& ) ) );
  connect( mView, SIGNAL( resetDate() ), SLOT( resetDate() ) );

  updateView();
}

DateEditWidget::~DateEditWidget()
{
}

void DateEditWidget::setDate( const QDate &date )
{
  mDate = date;
  mMenu->setDate( mDate );
  updateView();
}

QDate DateEditWidget::date() const
{
  return mDate;
}

void DateEditWidget::setReadOnly( bool readOnly )
{
  mButton->setEnabled( !readOnly );
}

void DateEditWidget::dateSelected(const QDate &date)
{
  mDate = date;
  updateView();
}

void DateEditWidget::resetDate()
{
  mDate = QDate();
  updateView();
}

void DateEditWidget::updateView()
{
  if ( mDate.isValid() )
    mView->setText( KGlobal::locale()->formatDate( mDate ) );
  else
    mView->setText( QString() );
}

#include "dateeditwidget.moc"
