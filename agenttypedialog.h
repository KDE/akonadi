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

#ifndef AKONADI_AGENTTYPEDIALOG_H
#define AKONADI_AGENTTYPEDIALOG_H

#include "agenttypewidget.h"
#include "agenttype.h"

#include <QtGui/QDialog>

namespace Akonadi {

/**
  * Shows a dialog with the AgentTypeWidget in there and an ok and cancel button.
  * @since 4.2
  */
class AKONADI_EXPORT AgentTypeDialog : public QDialog
{
  Q_OBJECT

  public:
    AgentTypeDialog( QWidget *parent = 0 );
    AgentType agentType() const;
    AgentFilterProxyModel* agentFilterProxyModel() const

  public Q_SLOTS:
    virtual void done( int result );

  private:
    // TODO toma: d-pointer before release.
    AgentTypeWidget *mWidget;
    AgentType mAgentType;
};

}

#endif
