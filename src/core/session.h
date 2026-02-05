/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include <QObject>

#include <memory>

class KJob;
class FakeSession;
class FakeNotificationConnection;

namespace Akonadi
{
namespace Protocol
{
class Command;
using CommandPtr = QSharedPointer<Command>;
}

class Job;
class SessionPrivate;
class ChangeNotificationDependenciesFactory;

/*!
 * \brief A communication session with the Akonadi storage.
 *
 * Every Job object has to be associated with a Session.
 * The session is responsible of scheduling its jobs.
 * For now only a simple serial execution is implemented (the IMAP-like
 * protocol to communicate with the storage backend is capable of parallel
 * execution on a single session though).
 *
 * \code
 *
 * using namespace Akonadi;
 *
 * Session *session = new Session( "mySession" );
 *
 * CollectionFetchJob *job = new CollectionFetchJob( Collection::root(),
 *                                                   CollectionFetchJob::Recursive,
 *                                                   session );
 *
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)) );
 *
 * \endcode
 *
 * \class Akonadi::Session
 * \inheaders Akonadi/Session
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT Session : public QObject
{
    Q_OBJECT

    friend class Job;
    friend class JobPrivate;
    friend class SessionPrivate;

public:
    /*!
     * Creates a new session.
     *
     * \a sessionId The identifier for this session, will be a
     *                  random value if empty.
     * \a parent The parent object.
     *
     * \sa defaultSession()
     */
    explicit Session(const QByteArray &sessionId = QByteArray(), QObject *parent = nullptr);

    /*!
     * Destroys the session.
     */
    ~Session() override;

    /*!
     * Returns the session identifier.
     */
    [[nodiscard]] QByteArray sessionId() const;

    /*!
     * Returns the default session for this thread.
     */
    static Session *defaultSession();

    /*!
     * Stops all jobs queued for execution.
     */
    void clear();

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the session has been reconnected
     * to the server (e.g. after a server crash).
     *
     * \since 4.6
     */
    void reconnected();

protected:
    /*!
     * Creates a new session with shared private object.
     *
     * \a d The private object.
     * \a sessionId The identifier for this session, will be a
     *                  random value if empty.
     * \a parent The parent object.
     *
     * \note This constructor is needed for unit testing only.
     */
    explicit Session(SessionPrivate *d, const QByteArray &sessionId = QByteArray(), QObject *parent = nullptr);

private:
    std::unique_ptr<SessionPrivate> const d;
    friend class ::FakeSession;
    friend class ::FakeNotificationConnection;
    friend class ChangeNotificationDependenciesFactory;

    Q_PRIVATE_SLOT(d, bool handleCommands())
};

}
