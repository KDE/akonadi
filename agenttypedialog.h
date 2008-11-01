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

#ifndef AKONADI_AGENTTYPEDIALOG_H
#define AKONADI_AGENTTYPEDIALOG_H

#include "agenttypewidget.h"
#include "agenttype.h"

#include <KDE/KDialog>

namespace Akonadi {

/**
  * Shows a dialog with the AgentTypeWidget in there and an ok and cancel button.
  * @since 4.2
  */
class AKONADI_EXPORT AgentTypeDialog : public KDialog
{
  Q_OBJECT

  public:
    AgentTypeDialog( QWidget *parent = 0 );
    ~AgentTypeDialog();
    AgentType agentType() const;
    AgentFilterProxyModel* agentFilterProxyModel() const;

  public Q_SLOTS:
    virtual void done( int result );

  private:
    class Private;
    Private * const d;
};

}

#endif
