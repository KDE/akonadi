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

#include "message.h"
#include "messagefetchjob.h"
#include "messagemodel.h"
#include "messagequery.h"
#include "monitor.h"

#include <kmime_message.h>

#include <kdebug.h>
#include <klocale.h>

using namespace PIM;

class MessageModel::Private
{
  public:
    QList<Message*> messages;
    QByteArray path;
    MessageFetchJob *listingJob;
    Monitor *monitor;
    QList<MessageQuery*> fetchJobs, updateJobs;
};

PIM::MessageModel::MessageModel( QObject *parent ) :
    QAbstractTableModel( parent ),
    d( new Private() )
{
  d->listingJob = 0;
  d->monitor = 0;
}

PIM::MessageModel::~MessageModel( )
{
  delete d->listingJob;
  delete d->monitor;
  qDeleteAll( d->fetchJobs );
  qDeleteAll( d->updateJobs );
  delete d;
}

int PIM::MessageModel::columnCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return 5; // keep in sync with the column type enum
}

QVariant PIM::MessageModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= d->messages.count() )
    return QVariant();
  Message* msg = d->messages.at( index.row() );
  Q_ASSERT( msg->mime() );
  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Subject:
        return msg->mime()->subject()->asUnicodeString();
      case Sender:
        return msg->mime()->from()->asUnicodeString();
      case Receiver:
        return msg->mime()->to()->asUnicodeString();
      case Date:
      case Size:
      // TODO
      default:
        return QVariant();
    }
  }
  return QVariant();
}

int PIM::MessageModel::rowCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return d->messages.count();
}

QVariant PIM::MessageModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case Subject:
        return i18n( "Subject" );
      case Sender:
        return i18n( "Sender" );
      case Receiver:
        return i18n( "Receiver" );
      case Date:
        return i18n( "Date" );
      case Size:
        return i18n( "Size" );
      default:
        return QString();
    }
  }
  return QAbstractTableModel::headerData( section, orientation, role );
}

void PIM::MessageModel::setPath( const QByteArray& path )
{
  if ( d->path == path )
    return;
  d->path = path;
  // the query changed, thus everything we have already is invalid
  d->messages.clear();
  reset();
  // stop all running jobs
  delete d->monitor;
  d->monitor = 0;
  qDeleteAll( d->updateJobs );
  d->updateJobs.clear();
  qDeleteAll( d->fetchJobs );
  d->updateJobs.clear();
  delete d->listingJob;
  // start listing job
  d->listingJob = new MessageFetchJob( path, this );
  connect( d->listingJob, SIGNAL( done( PIM::Job* ) ), SLOT( listingDone( PIM::Job* ) ) );
  d->listingJob->start();
}

void PIM::MessageModel::listingDone( PIM::Job * job )
{
  Q_ASSERT( job == d->listingJob );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Message query failed!" << endl;
  } else {
    d->messages = d->listingJob->messages();
    reset();
  }
  d->listingJob->deleteLater();
  d->listingJob = 0;

  // start monitor job
  // TODO error handling
  d->monitor = new Monitor( "folder=" + d->path );
  connect( d->monitor, SIGNAL( changed( const DataReference::List& ) ),
           SLOT( messagesChanged( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( added( const DataReference::List& ) ),
           SLOT( messagesAdded( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( removed( const DataReference::List& ) ),
           SLOT( messagesRemoved( const DataReference::List& ) ) );
  d->monitor->start();
}

void PIM::MessageModel::fetchingNewDone( PIM::Job * job )
{
  Q_ASSERT( d->fetchJobs.contains( static_cast<MessageQuery*>( job ) ) );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Fetching new messages failed!" << endl;
  } else {
    Message::List list = static_cast<MessageQuery*>( job )->messages();
    beginInsertRows( QModelIndex(), d->messages.size(), d->messages.size() + list.size() );
    d->messages += list;
    endInsertRows();
  }
  d->fetchJobs.removeAll( static_cast<MessageQuery*>( job ) );
  job->deleteLater();
}

void PIM::MessageModel::fetchingUpdatesDone( PIM::Job * job )
{
  Q_ASSERT( d->updateJobs.contains( static_cast<MessageQuery*>( job ) ) );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Updating changed messages failed!" << endl;
  } else {
    Message::List list = static_cast<MessageQuery*>( job )->messages();
    foreach ( Message* msg, list ) {
      // ### *slow*
      for ( int i = 0; i < d->messages.size(); ++i ) {
        if ( d->messages.at( i )->reference() == msg->reference() ) {
          delete d->messages.at( i );
          d->messages.replace( i, msg );
          emit dataChanged( index( i, 0 ), index( i, columnCount() ) );
          break;
        }
      }
    }
  }
  d->updateJobs.removeAll( static_cast<MessageQuery*>( job ) );
  job->deleteLater();
}

void PIM::MessageModel::messagesChanged( const DataReference::List & references )
{
  // TODO: build query based on the reference list
  QString query;
  MessageQuery* job = new MessageQuery( query );
  connect( job, SIGNAL( done( PIM::Job* ) ), SLOT( fetchingUpdatesDone( PIM::Job* job ) ) );
  job->start();
  d->updateJobs.append( job );
}

void PIM::MessageModel::messagesAdded( const DataReference::List & references )
{
  // TODO: build query based on the reference list
  QString query;
  MessageQuery* job = new MessageQuery( query );
  connect( job, SIGNAL( done( PIM::Job* ) ), SLOT( fetchingNewDone( PIM::Job* job ) ) );
  job->start();
  d->fetchJobs.append( job );
}

void PIM::MessageModel::messagesRemoved( const DataReference::List & references )
{
  foreach ( DataReference ref, references ) {
    // ### *slow*
    int index = -1;
    for ( int i = 0; i < d->messages.size(); ++i ) {
      if ( d->messages.at( i )->reference() == ref ) {
        index = i;
        break;
      }
    }
    if ( index < 0 )
      continue;
    beginRemoveRows( QModelIndex(), index, index );
    Message* msg = d->messages.at( index );
    d->messages.removeAt( index );
    delete msg;
    endRemoveRows();
  }
}

#include "messagemodel.moc"
