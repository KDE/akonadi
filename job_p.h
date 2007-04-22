/*
    This file is part of libakonadi.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_JOB_P_H
#define AKONADI_JOB_P_H

#include "session.h"

namespace Akonadi {

class Job::Private
{
  public:
    Private( Job *parent )
      : mParent( parent )
    {
    }

    void handleResponse( const QByteArray &tag, const QByteArray &data );
    void startQueued();
    void lostConnection();
    void slotSubJobAboutToStart( Akonadi::Job* );
    void startNext();

    Job *mParent;
    Job *mParentJob;
    Job *mCurrentSubJob;
    QByteArray mTag;
    Session* mSession;
};

}

#endif
