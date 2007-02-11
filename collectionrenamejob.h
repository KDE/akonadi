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

#ifndef AKONADI_COLLECTIONRENAMEJOB_H
#define AKONADI_COLLECTIONRENAMEJOB_H

#include <libakonadi/job.h>

namespace Akonadi {

class CollectionRenameJobPrivate;

/**
  Job to rename/move a collection.
*/
class AKONADI_EXPORT CollectionRenameJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new rename job.
      @param from Collection source path.
      @param to Collection destination path.
      @param parent The parent object.
    */
    CollectionRenameJob( const QString &from, const QString &to, QObject *parent = 0 );

    /**
      Destroys the current.
    */
    ~CollectionRenameJob();

  protected:
    virtual void doStart();

  private:
    CollectionRenameJobPrivate* const d;
};

}

#endif
