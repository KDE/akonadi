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

#include <QObject>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>

using namespace Akonadi;

AgentTypeDialog::AgentTypeDialog( QWidget *parent )
      : QDialog( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mWidget = new Akonadi::AgentTypeWidget( this );
  connect( mWidget, SIGNAL( doubleClicked() ), this, SLOT( accept() ) );

  QDialogButtonBox *box = new QDialogButtonBox( this );

  layout->addWidget( mWidget );
  layout->addWidget( box );

  QPushButton *ok = box->addButton( QDialogButtonBox::Ok );
  connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );

  QPushButton *cancel = box->addButton( QDialogButtonBox::Cancel );
  connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

  resize( 450, 320 );
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

#include "agenttypedialog.moc"
