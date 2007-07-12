/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "messagethreaderproxymodel.h"

#include "messagemodel.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

class MessageThreaderProxyModel::Private
{
  public:
    Private( MessageThreaderProxyModel *parent )
      : mParent( parent )
    {
    }

  void setCollection( Collection &col )
  {
    // Change here the collection of the message agent

    // what if there is already a mail threading agent running for this collection
    // and how to detect that ?
  }

  MessageThreaderProxyModel *mParent;
};

MessageThreaderProxyModel::MessageThreaderProxyModel( QObject *parent )
  : QAbstractProxyModel( parent ),
    d( new Private( this ) )
{
  connect( parent, SIGNAL( collectionChanged(Collection &collection) ), this, SLOT( setCollection(Collection &collection) ) );

  // start a mailthreader agent
}

MessageThreaderProxyModel::~MessageThreaderProxyModel()
{
  delete d;
}

#include "messagethreaderproxymodel.moc"
