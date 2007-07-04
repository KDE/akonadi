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

#include <libakonadi/imapparser.h>

using namespace Akonadi;

class SearchCreateJob::Private
{
  public:
    QString name;
    QString query;
};

SearchCreateJob::SearchCreateJob(const QString & name, const QString & query, QObject * parent) :
    Job( parent ), d( new Private )
{
  d->name = name;
  d->query = query;
}

SearchCreateJob::~ SearchCreateJob()
{
  delete d;
}

void SearchCreateJob::doStart()
{
  QByteArray command = newTag() + " SEARCH_STORE ";
  command += ImapParser::quote( d->name.toUtf8() );
  command += ' ';
  command += ImapParser::quote( d->query.toUtf8() );
  command += '\n';
  writeData( command );
}

#include "searchcreatejob.moc"
