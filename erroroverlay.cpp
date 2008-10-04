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

#include "erroroverlay.h"
#include "servermanager.h"
#include "selftestdialog.h"

#include <KDebug>
#include <KIcon>
#include <KLocale>

#include <QBoxLayout>
#include <QEvent>
#include <QLabel>
#include <QPalette>

using namespace Akonadi;

class ErrorOverlayStatic
{
  public:
    QList<QPair<QPointer<QWidget>, QPointer<QWidget> > > baseWidgets;
};

K_GLOBAL_STATIC( ErrorOverlayStatic, sInstance )

static bool isParentOf( QObject* o1, QObject* o2 )
{
  if ( !o1 || !o2 )
    return false;
  if ( o1 == o2 )
    return true;
  return isParentOf( o1, o2->parent() );
}

ErrorOverlay::ErrorOverlay( QWidget *baseWidget, QWidget * parent ) :
    QWidget( parent ? parent : baseWidget->window() ),
    mBaseWidget( baseWidget )
{
  Q_ASSERT( baseWidget );
  Q_ASSERT( parentWidget() != baseWidget );

  // check existing overlays to detect cascading
  for ( QList<QPair< QPointer<QWidget>, QPointer<QWidget> > >::Iterator it = sInstance->baseWidgets.begin();
        it != sInstance->baseWidgets.end(); ) {
    if ( (*it).first == 0 || (*it).second == 0 ) {
      // garbage collection
      it = sInstance->baseWidgets.erase( it );
      continue;
    }
    if ( isParentOf( (*it).first, baseWidget ) ) {
      // parent already has an overlay, kill ourselves
      mBaseWidget = 0;
      hide();
      deleteLater();
      return;
    }
    if ( isParentOf( baseWidget, (*it).first ) ) {
      // child already has overlay, kill that one
      delete (*it).second;
      it = sInstance->baseWidgets.erase( it );
      continue;
    }
    ++it;
  }
  sInstance->baseWidgets.append( qMakePair( mBaseWidget, QPointer<QWidget>( this ) ) );

  connect( baseWidget, SIGNAL(destroyed()), SLOT(deleteLater()) );
  mPreviousState = mBaseWidget->isEnabled();

  QBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->addStretch();
  mIconLabel = new QLabel( this );
  mIconLabel->setPixmap( KIcon( "dialog-error" ).pixmap( 64 ) );
  mIconLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  topLayout->addWidget( mIconLabel );

  mDescLabel = new QLabel( this );
  mDescLabel->setText( i18n( "<p style=\"color: white;\"><b>Akonadi not operational.<br/>"
      "<a href=\"details\" style=\"color: white;\">Details...</a></b></p>" ) );
  mDescLabel->setWordWrap( true );
  mDescLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  connect( mDescLabel, SIGNAL(linkActivated(QString)), SLOT(linkActivated()) );
  topLayout->addWidget( mDescLabel );
  topLayout->addStretch();

  setToolTip( i18n( "The Akonadi personal information management framework is not operational.\n"
      "Click on \"Details...\" to obtain detailed information on this problem." ) );

  mOverlayActive = ServerManager::isRunning();
  if ( mOverlayActive )
    started();
  else
    stopped();
  connect( ServerManager::instance(), SIGNAL(started()), SLOT(started()) );
  connect( ServerManager::instance(), SIGNAL(stopped()), SLOT(stopped()) );

  QPalette p = palette();
  p.setColor( backgroundRole(), QColor( 0, 0, 0, 128 ) );
  setPalette( p );
  setAutoFillBackground( true );

  mBaseWidget->installEventFilter( this );

  reposition();
}

ErrorOverlay::~ ErrorOverlay()
{
  if ( mBaseWidget )
    mBaseWidget->setEnabled( mPreviousState );
}

void ErrorOverlay::reposition()
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
  if ( mOverlayActive )
    show();

  // follow position changes
  const QPoint topLevelPos = mBaseWidget->mapTo( window(), QPoint( 0, 0 ) );
  const QPoint parentPos = parentWidget()->mapFrom( window(), topLevelPos );
  move( parentPos );

  // follow size changes
  // TODO: hide/scale icon if we don't have enough space
  resize( mBaseWidget->size() );
}

bool ErrorOverlay::eventFilter(QObject * object, QEvent * event)
{
  if ( object == mBaseWidget && mOverlayActive &&
    ( event->type() == QEvent::Move || event->type() == QEvent::Resize ||
      event->type() == QEvent::Show || event->type() == QEvent::Hide ||
      event->type() == QEvent::ParentChange ) ) {
    reposition();
  }
  return QWidget::eventFilter( object, event );
}

void ErrorOverlay::linkActivated()
{
  SelfTestDialog dlg;
  dlg.exec();
}

void ErrorOverlay::started()
{
  if ( !mBaseWidget )
    return;
  mOverlayActive = false;
  hide();
  mBaseWidget->setEnabled( mPreviousState );
}

void ErrorOverlay::stopped()
{
  if ( !mBaseWidget )
    return;
  mOverlayActive = true;
  if ( mBaseWidget->isVisible() )
    show();
  mPreviousState = mBaseWidget->isEnabled();
  mBaseWidget->setEnabled( false );
  reposition();
}

#include "erroroverlay.moc"
