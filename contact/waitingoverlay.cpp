/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "waitingoverlay_p.h"

#include <KDebug>
#include <KIcon>
#include <KJob>
#include <KLocale>

#include <QtCore/QEvent>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPalette>
#include <QtGui/QProgressBar>

//@cond PRIVATE

WaitingOverlay::WaitingOverlay( KJob *job, QWidget *baseWidget, QWidget * parent )
  : QWidget( parent ? parent : baseWidget->window() ),
    mBaseWidget( baseWidget )
{
  Q_ASSERT( baseWidget );
  Q_ASSERT( parentWidget() != baseWidget );

  connect( baseWidget, SIGNAL( destroyed() ), SLOT( deleteLater() ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( deleteLater() ) );
  mPreviousState = mBaseWidget->isEnabled();

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->addStretch();
  mDescription = new QLabel( this );
  mDescription->setText( i18n( "<p style=\"color: white;\"><b>Waiting for operation</b><br/></p>" ) );
  mDescription->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  topLayout->addWidget( mDescription );
  topLayout->addStretch();

  QPalette p = palette();
  p.setColor( backgroundRole(), QColor( 0, 0, 0, 128 ) );
  setPalette( p );
  setAutoFillBackground( true );

  mBaseWidget->installEventFilter( this );

  reposition();
}

WaitingOverlay::~ WaitingOverlay()
{
  if ( mBaseWidget )
    mBaseWidget->setEnabled( mPreviousState );
}

void WaitingOverlay::reposition()
{
  if ( !mBaseWidget )
    return;

  // reparent to the current top level widget of the base widget if needed
  // needed eg. in dock widgets
  if ( parentWidget() != mBaseWidget->window() )
    setParent( mBaseWidget->window() );

  // follow base widget visibility
  // needed eg. in tab widgets
  if ( !mBaseWidget->isVisible() ) {
    hide();
    return;
  }
  show();

  // follow position changes
  const QPoint topLevelPos = mBaseWidget->mapTo( window(), QPoint( 0, 0 ) );
  const QPoint parentPos = parentWidget()->mapFrom( window(), topLevelPos );
  move( parentPos );

  // follow size changes
  // TODO: hide/scale icon if we don't have enough space
  resize( mBaseWidget->size() );
}

bool WaitingOverlay::eventFilter(QObject * object, QEvent * event)
{
  if ( object == mBaseWidget &&
    ( event->type() == QEvent::Move || event->type() == QEvent::Resize ||
      event->type() == QEvent::Show || event->type() == QEvent::Hide ||
      event->type() == QEvent::ParentChange ) ) {
    reposition();
  }
  return QWidget::eventFilter( object, event );
}

//@endcond
