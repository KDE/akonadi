/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_COLLECTIONSELECTJOB_H
#define PIM_COLLECTIONSELECTJOB_H

#include <libakonadi/job.h>

namespace PIM {

class CollectionSelectJobPrivate;

/**
  Selects a specific collection. See RFC 3501 for select semantics.
*/
class AKONADI_EXPORT CollectionSelectJob : public Job
{
  Q_OBJECT
  public:
    /**
      Creates a new collection select job.
      @param path The absolute path of the collection to select.
      @param parent The parent object.
    */
    CollectionSelectJob( const QByteArray &path, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionSelectJob();

    /**
      Returns the unseen count of the selected folder, -1 if not available.
    */
    int unseen() const;

  protected:
    void doStart();
    void handleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionSelectJobPrivate *d;
};

}

#endif
