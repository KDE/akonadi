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

#include "tracerinterface.h"

ConnectionPage::ConnectionPage( const QString &identifier, QWidget *parent )
  : QWidget( parent ), mIdentifier( identifier )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mDataView = new QTextEdit( this );
  mDataView->setReadOnly( true );

  layout->addWidget( mDataView );

  org::kde::Akonadi::Tracer *iface = new org::kde::Akonadi::Tracer( QString(), "/tracing", QDBus::sessionBus(), this );

  connect( iface, SIGNAL( connectionDataInput( const QString&, const QString& ) ),
           this, SLOT( connectionDataInput( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionDataOutput( const QString&, const QString& ) ),
           this, SLOT( connectionDataOutput( const QString&, const QString& ) ) );
}

void ConnectionPage::connectionDataInput( const QString &identifier, const QString &msg )
{
  if ( identifier == mIdentifier )
    mDataView->append( QString( "<font color=\"red\">%1</font><br>" ).arg( msg ) );
}

void ConnectionPage::connectionDataOutput( const QString &identifier, const QString &msg )
{
  if ( identifier == mIdentifier )
    mDataView->append( QString( "<font color=\"blue\">%1</font><br>" ).arg( msg ) );
}

#include "connectionpage.moc"
