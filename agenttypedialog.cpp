/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"

#include <QObject>
#include <QtGui/QVBoxLayout>

#include <kdeversion.h>

#if KDE_IS_VERSION(4,1,85)
#include <kfilterproxysearchline.h>
#endif

using namespace Akonadi;


class AgentTypeDialog::Private
{
  public:
    AgentTypeWidget *Widget;
    AgentType agentType;
};

AgentTypeDialog::AgentTypeDialog( QWidget *parent )
      : KDialog( parent ), d( new Private )
{
  setButtons( Ok | Cancel );
  QVBoxLayout *layout = new QVBoxLayout( mainWidget() );

  d->Widget = new Akonadi::AgentTypeWidget( mainWidget() );
  connect( d->Widget, SIGNAL( activated() ), this, SLOT( accept() ) );

#if KDE_IS_VERSION(4,1,85)
  KFilterProxySearchLine* searchLine = new KFilterProxySearchLine( mainWidget() );
  layout->addWidget( searchLine );
  searchLine->setProxy( d->Widget->agentFilterProxyModel() );
#endif

  layout->addWidget( d->Widget );

  connect( this, SIGNAL( okClicked() ), this, SLOT( accept() ) );

  resize( 460, 320 );
}

AgentTypeDialog::~AgentTypeDialog()
{
  delete d;
}

void AgentTypeDialog::done( int result )
{
  if ( result == Accepted )
    d->agentType = d->Widget->currentAgentType();
  else
    d->agentType = AgentType();

  KDialog::done( result );
}

AgentType AgentTypeDialog::agentType() const
{
  return d->agentType;
}

AgentFilterProxyModel* AgentTypeDialog::agentFilterProxyModel() const
{
  return d->Widget->agentFilterProxyModel();
}

#include "agenttypedialog.moc"
