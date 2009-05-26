/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "searchcreatejob.h"

#include "imapparser_p.h"
#include "job_p.h"

using namespace Akonadi;

class Akonadi::SearchCreateJobPrivate : public JobPrivate
{
  public:
    SearchCreateJobPrivate( SearchCreateJob *parent )
      : JobPrivate( parent )
    {
    }

    QString mName;
    QString mQuery;
};

SearchCreateJob::SearchCreateJob(const QString & name, const QString & query, QObject * parent)
  : Job( new SearchCreateJobPrivate( this ), parent )
{
  Q_D( SearchCreateJob );

  d->mName = name;
  d->mQuery = query;
}

SearchCreateJob::~SearchCreateJob()
{
}

void SearchCreateJob::doStart()
{
  Q_D( SearchCreateJob );

  QByteArray command = d->newTag() + " SEARCH_STORE ";
  command += ImapParser::quote( d->mName.toUtf8() );
  command += ' ';
  command += ImapParser::quote( d->mQuery.toUtf8() );
  command += '\n';
  d->writeData( command );
}

#include "searchcreatejob.moc"
