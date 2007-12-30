/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SUBSCRIPTIONJOB_H
#define AKONADI_SUBSCRIPTIONJOB_H

#include <libakonadi/job.h>
#include <libakonadi/collection.h>

namespace Akonadi {

/**
  Job to manipulate the local subscription state of a set of collections.
*/
class AKONADI_EXPORT SubscriptionJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new subscription job.
      @param parent The parent object.
    */
    explicit SubscriptionJob( QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~SubscriptionJob();

    /**
      Subscribe to the given list of collections.
      @param list List of collections to subscribe to.
    */
    void subscribe( const Collection::List &list );

    /**
      Unsubscribe from the given list of collections.
      @param list List of collections to unsubscribe from.
    */
    void unsubscribe( const Collection::List &list );

  protected:
    void doStart();
    void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    class Private;
    Private* const d;
};

}

#endif
