/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "transactionjobs.h"

#include "job_p.h"

using namespace Akonadi;

class Akonadi::TransactionBeginJobPrivate : public JobPrivate
{
  public:
    TransactionBeginJobPrivate( TransactionBeginJob *parent )
      : JobPrivate( parent )
    {
    }
};

class Akonadi::TransactionRollbackJobPrivate : public JobPrivate
{
  public:
    TransactionRollbackJobPrivate( TransactionRollbackJob *parent )
      : JobPrivate( parent )
    {
    }
};

class Akonadi::TransactionCommitJobPrivate : public JobPrivate
{
  public:
    TransactionCommitJobPrivate( TransactionCommitJob *parent )
      : JobPrivate( parent )
    {
    }
};


TransactionBeginJob::TransactionBeginJob(QObject * parent)
  : Job( new TransactionBeginJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionBeginJob::~TransactionBeginJob()
{
}

void TransactionBeginJob::doStart()
{
  writeData( newTag() + " BEGIN\n" );
}



TransactionRollbackJob::TransactionRollbackJob(QObject * parent)
  : Job( new TransactionRollbackJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionRollbackJob::~TransactionRollbackJob()
{
}

void TransactionRollbackJob::doStart()
{
  writeData( newTag() + " ROLLBACK\n" );
}



TransactionCommitJob::TransactionCommitJob(QObject * parent)
  : Job( new TransactionCommitJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionCommitJob::~TransactionCommitJob()
{
}

void TransactionCommitJob::doStart()
{
  writeData( newTag() + " COMMIT\n" );
}

#include "transactionjobs.moc"
