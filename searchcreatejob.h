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

#ifndef AKONADI_SEARCHCREATEJOB_H
#define AKONADI_SEARCHCREATEJOB_H

#include <libakonadi/job.h>

namespace Akonadi {

/**
  Creates a virtual/search collection, ie. a collection
  containing search results.

  @todo Add method that returns the new collection.
*/
class AKONADI_EXPORT SearchCreateJob : public Job
{
  Q_OBJECT
  public:
    /**
      Creates a virtual/search collection.
      @param name The name of the collection.
      @param query The XESAM query.
      @param parent The parent object.
    */
    SearchCreateJob( const QString &name, const QString &query, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~SearchCreateJob();

  protected:
    void doStart();

  private:
    class Private;
    Private* const d;
};

}

#endif
