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

#ifndef AKONADI_EXPUNGEJOB_H
#define AKONADI_EXPUNGEJOB_H

#include <libakonadi/job.h>

namespace Akonadi {

class ExpungeJobPrivate;

/**
  Permanently removes all items marked for deletion.
*/
class ExpungeJob : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new ExpungeJob.
      @param parent The parent object.
    */
    ExpungeJob( QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~ExpungeJob();

  protected:
    virtual void doStart();

  private:
    ExpungeJobPrivate *d;

};

}

#endif
