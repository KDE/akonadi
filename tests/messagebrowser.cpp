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
#include "messagebrowser.h"
#include "messagemodel.h"

#include "kmime_message.h"
#include <QDebug>
#include <QTextEdit>

using namespace PIM;

MessageBrowser::MessageBrowser( const QByteArray &path ) : QTreeView()
{
  MessageModel* model = new MessageModel( this );
  model->setPath( path );
  setModel( model );

  connect( this, SIGNAL(doubleClicked(QModelIndex)), SLOT(messageActivated(QModelIndex)) );
}

void PIM::MessageBrowser::messageActivated( const QModelIndex & index )
{
  DataReference ref = static_cast<MessageModel*>( model() )->referenceForIndex( index );
  if ( ref.isNull() )
    return;
  MessageFetchJob *job = new MessageFetchJob( ref, this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(slotFetchDone(PIM::Job*)) );
  job->start();
}

void PIM::MessageBrowser::slotFetchDone( PIM::Job * job )
{
  if ( job->error() ) {
    qWarning() << "Message fetch failed: " << job->errorText();
  } else {
    Message *msg = static_cast<MessageFetchJob*>( job )->messages().first();
    QTextEdit *te = new QTextEdit();
    te->setReadOnly( true );
    te->setPlainText( msg->mime()->encodedContent() );
    te->show();
  }
  job->deleteLater();
}

#include "messagebrowser.moc"
