/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "fakeconnection.h"
#include "imapstreamparser.h"
#include "response.h"
#include "fakedatastore.h"
#include "handler/scope.h"

#include <QBuffer>

using namespace Akonadi::Server;

FakeConnection::FakeConnection()
  : Connection()
  , mHandler(0)
{
}

FakeConnection::~FakeConnection()
{

}

DataStore* FakeConnection::storageBackend()
{
    if (!m_backend) {
        m_backend = static_cast<FakeDataStore*>(FakeDataStore::self());
    }

    return m_backend;
}

void FakeConnection::setCommand(const QByteArray &command)
{
    mData = command;
    QBuffer *buffer = new QBuffer(&mData);
    buffer->open(QIODevice::ReadOnly);
    m_socket = buffer;
    m_streamParser = new ImapStreamParser(m_socket);
}

void FakeConnection::setHandler(Handler* handler)
{
    mHandler = handler;
}

void FakeConnection::setContext(const CommandContext& context)
{
    m_context = context;
}

Handler* FakeConnection::findHandlerForCommand(const QByteArray& command)
{
    Q_ASSERT_X(mHandler, "FakeConnection::findHandlerForCommand",
               "No handler set");

    // Position m_streamParser after the actual command if there was a scope
    Scope::SelectionScope scope = Scope::selectionScopeFromByteArray(command);
    if ( scope != Scope::None ) {
        m_streamParser->readString();
    }

    return mHandler;
}

void FakeConnection::slotResponseAvailable(const Response &response)
{
    Q_EMIT responseAvailable(response);
}

void FakeConnection::run()
{
    slotNewData();
}
