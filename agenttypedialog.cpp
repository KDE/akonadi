/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"

#include <QObject>
#include <QtGui/QVBoxLayout>

#include <kfilterproxysearchline.h>

using namespace Akonadi;

AgentTypeDialog::AgentTypeDialog( QWidget *parent )
      : KDialog( parent )
{
  setButtons( Ok | Cancel );
  QVBoxLayout *layout = new QVBoxLayout( mainWidget() );

  KFilterProxySearchLine* searchLine = new KFilterProxySearchLine( mainWidget() );
  layout->addWidget( searchLine );

  mWidget = new Akonadi::AgentTypeWidget( mainWidget() );
  connect( mWidget, SIGNAL( activated() ), this, SLOT( accept() ) );

  searchLine->setProxy( mWidget->agentFilterProxyModel() );

  layout->addWidget( mWidget );

  connect( this, SIGNAL( okClicked() ), this, SLOT( accept() ) );

  resize( 460, 320 );
}

AgentTypeDialog::~AgentTypeDialog()
{
}

void AgentTypeDialog::done( int result )
{
  if ( result == Accepted )
    mAgentType = mWidget->currentAgentType();
  else
    mAgentType = AgentType();

  QDialog::done( result );
}

AgentType AgentTypeDialog::agentType() const
{
  return mAgentType;
}

AgentFilterProxyModel* AgentTypeDialog::agentFilterProxyModel() const
{
  return mWidget->agentFilterProxyModel();
}


#include "agenttypedialog.moc"
