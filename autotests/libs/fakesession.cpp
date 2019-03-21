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
#include "private/protocol_p.h"

#include <QCoreApplication>
#include <QTimer>

class FakeSessionPrivate : public SessionPrivate
{
public:
    FakeSessionPrivate(FakeSession *parent, FakeSession::Mode mode)
        : SessionPrivate(parent), q_ptr(parent), m_mode(mode)
    {
        protocolVersion = Protocol::version();
    }

    /* reimp */
    void init(const QByteArray &id) override
    {
        // trimmed down version of the real SessionPrivate::init(), without any server access
        if (!id.isEmpty()) {
            sessionId = id;
        } else {
            sessionId = QCoreApplication::instance()->applicationName().toUtf8()
            + '-' + QByteArray::number(qrand());
        }

        connected = false;
        theNextTag = 1;
        jobRunning = false;

        reconnect();
    }

    /* reimp */
    void reconnect() override
    {
        if (m_mode == FakeSession::EndJobsImmediately) {
            return;
        }

        // Like Session does: delay the actual disconnect+reconnect
        QTimer::singleShot(10, q_ptr, [&]() {
            socketDisconnected();
            Q_EMIT q_ptr->reconnected();
            connected = true;
            startNext();
        });
    }

    /* reimp */
    void addJob(Job *job) override
    {
        Q_EMIT q_ptr->jobAdded(job);
        // Return immediately so that no actual communication happens with the server and
        // the started jobs are completed.
        if (m_mode == FakeSession::EndJobsImmediately) {
            endJob(job);
        } else {
            SessionPrivate::addJob(job);
        }
    }

    FakeSession *q_ptr;
    FakeSession::Mode m_mode;
};

FakeSession::FakeSession(const QByteArray &sessionId, FakeSession::Mode mode, QObject *parent)
    : Session(new FakeSessionPrivate(this, mode), sessionId, parent)
{

}

void FakeSession::setAsDefaultSession()
{
    d->setDefaultSession(this);
}
