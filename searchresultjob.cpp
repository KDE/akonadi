/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "searchresultjob_p.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <akonadi/private/protocol_p.h>



namespace Akonadi {

class SearchResultJobPrivate: public Akonadi::JobPrivate
{
  public:
    SearchResultJobPrivate( SearchResultJob *parent );

    QByteArray searchId;
    Collection collection;
    ImapSet uid;
    QVector<QByteArray> rid;
};

SearchResultJobPrivate::SearchResultJobPrivate( SearchResultJob *parent)
 : JobPrivate( parent )
{
}

}


using namespace Akonadi;

SearchResultJob::SearchResultJob( const QByteArray &searchId, const Collection &collection, QObject *parent )
 : Job(new SearchResultJobPrivate( this ), parent )
{
  Q_D( SearchResultJob );
  Q_ASSERT( collection.isValid() );

  d->searchId = searchId;
  d->collection = collection;
}

SearchResultJob::~SearchResultJob()
{
}

void SearchResultJob::setSearchId( const QByteArray &searchId )
{
  Q_D( SearchResultJob );
  d->searchId = searchId;
}

QByteArray SearchResultJob::searchId() const
{
  return d_func()->searchId;
}

void SearchResultJob::setResult( const ImapSet &set )
{
  Q_D( SearchResultJob );
  d->rid.clear();
  d->uid = set;
}

void SearchResultJob::setResult( const QVector<qint64> &ids )
{
  Q_D( SearchResultJob );
  d->rid.clear();
  d->uid = ImapSet();
  d->uid.add( ids );
}

void SearchResultJob::setResult( const QVector<QByteArray> &remoteIds )
{
  Q_D( SearchResultJob );
  d->uid = ImapSet();
  d->rid = remoteIds;
}

void SearchResultJob::doStart()
{
  Q_D( SearchResultJob );

  QByteArray command = d->newTag() + ' ';

  if ( !d->rid.isEmpty() ) {
    command += AKONADI_CMD_RID;
  } else {
    command += AKONADI_CMD_UID;
  }

  command += " SEARCH_RESULT " + d->searchId + " " + QByteArray::number( d->collection.id() ) + " (";

  if ( !d->rid.isEmpty() ) {
    command += ImapParser::join( d->rid.toList(), " " );
  } else if ( !d->uid.isEmpty() ) {
    command += d->uid.toImapSequenceSet();
  }

  command += ")";

  d->writeData( command );
}
