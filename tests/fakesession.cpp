/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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



#include "fakesession.h"
#include "session_p.h"
#include "job.h"

#include <KDebug>

class FakeSessionPrivate : public SessionPrivate
{
  public:
    FakeSessionPrivate(Session* parent)
      : SessionPrivate( parent )
    {

    }

    /* reimp */ void init() {}

    /* reimp */ void addJob( Job *job )
    {
      // Return immediately so that no actual communication happens with the server and
      // the started jobs are completed.
      endJob( job );
    }

};

FakeSession::FakeSession(const QByteArray& sessionId, QObject* parent)
    : Session(new FakeSessionPrivate(this), sessionId, parent)
{

}

void FakeSession::firstListJobResult(QList<Collection::List> collectionChunks)
{
  // write the collection chunks to the socket.
}



