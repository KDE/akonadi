/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>

#include "connectionpage.h"

#include "tracernotificationinterface.h"

ConnectionPage::ConnectionPage( const QString &identifier, QWidget *parent )
  : QWidget( parent ), mIdentifier( identifier ), mShowAllConnections( false )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mDataView = new QTextEdit( this );
  mDataView->setReadOnly( true );

  layout->addWidget( mDataView );

  org::kde::Akonadi::TracerNotification *iface = new org::kde::Akonadi::TracerNotification( QString(), "/tracing/notifications", QDBusConnection::sessionBus(), this );

  connect( iface, SIGNAL( connectionDataInput( const QString&, const QString& ) ),
           this, SLOT( connectionDataInput( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionDataOutput( const QString&, const QString& ) ),
           this, SLOT( connectionDataOutput( const QString&, const QString& ) ) );
}

void ConnectionPage::connectionDataInput( const QString &identifier, const QString &msg )
{
  QString str;
  if ( mShowAllConnections ) {
    str += identifier + " ";
  }
  if ( mShowAllConnections || identifier == mIdentifier ) {
    str += QString( "<font color=\"red\">%1</font><br>" ).arg( msg );
    mDataView->append( str );
  }
}

void ConnectionPage::connectionDataOutput( const QString &identifier, const QString &msg )
{
  QString str;
  if ( mShowAllConnections ) {
    str += identifier + " ";
  }
  if ( mShowAllConnections || identifier == mIdentifier ) {
    str += QString( "<font color=\"blue\">%1</font><br>" ).arg( msg );
    mDataView->append( str );
  }
}

void ConnectionPage::showAllConnections( bool show )
{
  mShowAllConnections = show;
}

#include "connectionpage.moc"
