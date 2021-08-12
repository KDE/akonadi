/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakesession.h"
#include "job.h"
#include "private/protocol_p.h"
#include "session_p.h"

#include <QCoreApplication>
#include <QTimer>
using namespace std::chrono_literals;
class FakeSessionPrivate : public SessionPrivate
{
public:
    FakeSessionPrivate(FakeSession *parent, FakeSession::Mode mode)
        : SessionPrivate(parent)
        , q_ptr(parent)
        , m_mode(mode)
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
            sessionId = QCoreApplication::instance()->applicationName().toUtf8() + '-' + QByteArray::number(qrand());
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
        QTimer::singleShot(10ms, q_ptr, [&]() {
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
