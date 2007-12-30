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

#include "subscriptionjob.h"

using namespace Akonadi;

class SubscriptionJob::Private
{
  public:
    Private( SubscriptionJob *parent ) : q( parent ) {}

    void sendCommand( const QByteArray &cmd, const Collection::List &list )
    {
      tag = q->newTag();
      QByteArray line = tag + " " + cmd;
      foreach ( const Collection col, list )
        line += " " + QByteArray::number( col.id() );
      line += '\n';
      q->writeData( line );
      q->newTag(); // prevent automatic response handling
    }

    void sendNextCommand()
    {
      QByteArray cmd;
      if ( !sub.isEmpty() ) {
        sendCommand( "SUBSCRIBE", sub );
        sub.clear();
      } else if ( !unsub.isEmpty() ) {
        sendCommand( "UNSUBSCRIBE", unsub );
        unsub.clear();
      } else {
        q->emitResult();
      }
    }

    SubscriptionJob* q;
    QByteArray tag;
    Collection::List sub, unsub;
};

SubscriptionJob::SubscriptionJob(QObject * parent) :
    Job( parent ),
    d( new Private( this ) )
{
}

SubscriptionJob::~ SubscriptionJob()
{
  delete d;
}

void SubscriptionJob::subscribe(const Collection::List & list)
{
  d->sub = list;
}

void SubscriptionJob::unsubscribe(const Collection::List & list)
{
  d->unsub = list;
}

void SubscriptionJob::doStart()
{
  d->sendNextCommand();
}

void SubscriptionJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  if ( _tag == d->tag ) {
    if ( data.startsWith( "OK" ) ) {
      d->sendNextCommand();
    } else {
      setError( Unknown );
      setErrorText( QString::fromUtf8( data ) );
      emitResult();
    }
    return;
  }
}
