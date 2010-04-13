
/*
    Copyright (c) 2010 Till Adam <adam@kde.org>

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

#include "searchmodifyjob.h"

#include "collection.h"
#include "imapparser_p.h"
#include "protocolhelper_p.h"
#include "job_p.h"

using namespace Akonadi;

class Akonadi::SearchModifyJobPrivate : public JobPrivate
{
  public:
    SearchModifyJobPrivate( SearchModifyJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mCollection;
    QString mQuery;
    Collection mModifiedCollection;
};

SearchModifyJob::SearchModifyJob( const Collection & col, const QString & query, QObject * parent )
  : Job( new SearchModifyJobPrivate( this ), parent )
{
  Q_D( SearchModifyJob );

  d->mCollection = col;
  d->mQuery = query;
}

SearchModifyJob::~SearchModifyJob()
{
}

void SearchModifyJob::doStart()
{
  Q_D( SearchModifyJob );

  QByteArray command = d->newTag() + " SEARCH_MODIFY ";
  command += QString::number( d->mCollection.id() ).toUtf8();
  command += ' ';
  command += ImapParser::quote( d->mQuery.toUtf8() );
  command += '\n';
  d->writeData( command );
}

Collection SearchModifyJob::modifiedCollection() const
{
  Q_D( const SearchModifyJob );
  return d->mModifiedCollection;
}

void SearchModifyJob::doHandleResponse( const QByteArray &tag, const QByteArray &data )
{
  Q_D( SearchModifyJob );
  if ( tag == "*" ) {
   ProtocolHelper::parseCollection( data, d->mModifiedCollection );
   return;
  }
  kDebug() << "Unhandled response: " << tag << data;
}

#include "searchmodifyjob.moc"
