/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_MESSAGEBROWSER_H
#define PIM_MESSAGEBROWSER_H

#include <QtGui/QTreeView>

#include <libakonadi/job.h>

namespace PIM {

class MessageBrowser : public QTreeView
{
  Q_OBJECT
  public:
    MessageBrowser( const QString &path );
  private Q_SLOTS:
    void messageActivated( const QModelIndex &index );
    void slotFetchDone( PIM::Job* job );
};

}

#endif
