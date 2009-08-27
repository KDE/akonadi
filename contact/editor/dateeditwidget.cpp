/*
    This file is part of Akonadi Contact.

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
#include <klocale.h>
#include <libkdepim-copy/kdatepickerpopup.h>

#include <QtGui/QContextMenuEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

DateView::DateView( QWidget *parent )
  : QLabel( parent )
{
  setTextInteractionFlags( Qt::TextSelectableByMouse );
  setFrameShape( QFrame::Panel );
  setFrameShadow( QFrame::Sunken );
}

void DateView::contextMenuEvent( QContextMenuEvent *event )
{
  QMenu menu;
  menu.addAction( i18n( "Remove" ), this, SLOT( emitSignal() ) );

  menu.exec( event->globalPos() );
}

void DateView::emitSignal()
{
  emit resetDate();
}

DateEditWidget::DateEditWidget( QWidget *parent )
  : QWidget( parent ), mReadOnly( false )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );

  mView = new DateView;
  layout->addWidget( mView );

  mClearButton = new QToolButton;
  if ( layoutDirection() == Qt::LeftToRight )
    mClearButton->setIcon( KIcon( QLatin1String( "edit-clear-locationbar-rtl" ) ) );
  else
    mClearButton->setIcon( KIcon( QLatin1String( "edit-clear-locationbar-ltr" ) ) );
  layout->addWidget( mClearButton );

  mSelectButton = new QToolButton;
  mSelectButton->setPopupMode( QToolButton::InstantPopup );
  mSelectButton->setIcon( KIcon( QLatin1String( "view-calendar-day" ) ) );
  layout->addWidget( mSelectButton );

  mMenu = new KPIM::KDatePickerPopup( KPIM::KDatePickerPopup::DatePicker, QDate(), this );
  mSelectButton->setMenu( mMenu );

  connect( mClearButton, SIGNAL( clicked() ), SLOT( resetDate() ) );
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
  mReadOnly = readOnly;

  mSelectButton->setEnabled( !readOnly );
  mClearButton->setEnabled( !readOnly );
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
  if ( mDate.isValid() ) {
    mView->setText( KGlobal::locale()->formatDate( mDate ) );
    mClearButton->show();
  } else {
    mView->setText( QString() );
    mClearButton->hide();
  }
}

#include "dateeditwidget.moc"
