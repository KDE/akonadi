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
#include <libakonadi/itemfetchjob.h>
#include <agents/mailthreader/mailthreaderagent.h>
#include <messagemodel.h>

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

  MessageModel* sourceMessageModel()
  {
    return dynamic_cast<MessageModel*>( mParent->sourceModel() );
  }

  MessageThreaderProxyModel *mParent;
};

MessageThreaderProxyModel::MessageThreaderProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{

}

MessageThreaderProxyModel::~MessageThreaderProxyModel()
{
  delete d;
}

// ### Awful ! Already in mailthreaderagent.cpp ; just for testing purposes, though
const QLatin1String MailThreaderAgent::PartParent = QLatin1String( "AkonadiMailThreaderAgentParent" );
const QLatin1String MailThreaderAgent::PartSort = QLatin1String( "AkonadiMailThreaderAgentSort");

QModelIndex MessageThreaderProxyModel::parent ( const QModelIndex & index ) const
{
  Item item = d->sourceMessageModel()->itemForIndex( mapToSource( index ) );
  bool ok;
  int parentId = item.part( MailThreaderAgent::PartParent ).toInt( &ok );
  if (!ok)
  {
    return QModelIndex();
  }
  else
    qDebug() << "threader proxy model : fount parent id" << parentId << " for "<< item.reference().id();
  // ### *slow*
  for ( int i = 0; i < rowCount(); ++i ) {
    qDebug() << parentId << d->sourceMessageModel()->data( d->sourceMessageModel()->index( i, 0 ) );
    if ( d->sourceMessageModel()->data( d->sourceMessageModel()->index( i, 0 ), ItemModel::Id ) == parentId )
    {

      return mapFromSource( d->sourceMessageModel()->index( i, 0 ) );
    }
  }

  return QModelIndex();
}

void MessageThreaderProxyModel::setSourceModel( QAbstractItemModel* model )
{
  // TODO Assert model is a MessageModel
  QSortFilterProxyModel::setSourceModel( model );

  d->sourceMessageModel()->addFetchPart( MailThreaderAgent::PartParent );
}
#include "messagethreaderproxymodel.moc"
