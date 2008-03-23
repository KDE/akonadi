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

#include "job_p.h"

using namespace Akonadi;

class Akonadi::SubscriptionJobPrivate : public JobPrivate
{
  public:
    SubscriptionJobPrivate( SubscriptionJob *parent )
      : JobPrivate( parent )
    {
    }

    void sendCommand( const QByteArray &cmd, const Collection::List &list )
    {
      Q_Q( SubscriptionJob );

      mTag = q->newTag();
      QByteArray line = mTag + ' ' + cmd;
      foreach ( const Collection col, list )
        line += ' ' + QByteArray::number( col.id() );
      line += '\n';
      q->writeData( line );
      q->newTag(); // prevent automatic response handling
    }

    void sendNextCommand()
    {
      Q_Q( SubscriptionJob );

      QByteArray cmd;
      if ( !mSub.isEmpty() ) {
        sendCommand( "SUBSCRIBE", mSub );
        mSub.clear();
      } else if ( !mUnsub.isEmpty() ) {
        sendCommand( "UNSUBSCRIBE", mUnsub );
        mUnsub.clear();
      } else {
        q->emitResult();
      }
    }

    Q_DECLARE_PUBLIC( SubscriptionJob )

    QByteArray mTag;
    Collection::List mSub, mUnsub;
};

SubscriptionJob::SubscriptionJob(QObject * parent)
  : Job( new SubscriptionJobPrivate( this ), parent )
{
}

SubscriptionJob::~SubscriptionJob()
{
}

void SubscriptionJob::subscribe(const Collection::List & list)
{
  Q_D( SubscriptionJob );

  d->mSub = list;
}

void SubscriptionJob::unsubscribe(const Collection::List & list)
{
  Q_D( SubscriptionJob );

  d->mUnsub = list;
}

void SubscriptionJob::doStart()
{
  Q_D( SubscriptionJob );

  d->sendNextCommand();
}

void SubscriptionJob::doHandleResponse(const QByteArray &_tag, const QByteArray & data)
{
  Q_D( SubscriptionJob );

  if ( _tag == d->mTag ) {
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

#include "subscriptionjob.moc"
