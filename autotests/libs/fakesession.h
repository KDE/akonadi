/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonaditestfake_export.h"
#include "collection.h"
#include "session.h"

using namespace Akonadi;

class AKONADITESTFAKE_EXPORT FakeSession : public Session
{
    Q_OBJECT
public:
    enum Mode {
        EndJobsImmediately,
        EndJobsManually,
    };

    explicit FakeSession(const QByteArray &sessionId = QByteArray(), Mode mode = EndJobsImmediately, QObject *parent = nullptr);

    /** Make this the default session returned by Akonadi::Session::defaultSession().
     *  Note that ownership is taken over by the thread-local storage.
     */
    void setAsDefaultSession();

Q_SIGNALS:
    void jobAdded(Akonadi::Job *job);

    friend class FakeSessionPrivate;
};
