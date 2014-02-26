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

#ifndef AKONADI_CACHECLEANER_H
#define AKONADI_CACHECLEANER_H

#include "collectionscheduler.h"

namespace Akonadi {
namespace Server {

class Collection;

/**
  Cache cleaner thread.
*/
class CacheCleaner : public CollectionScheduler
{
  Q_OBJECT

  public:
    /**
      Creates a new cache cleaner thread.
      @param parent The parent object.
    */
    CacheCleaner( QObject *parent = 0 );
    ~CacheCleaner();

    static CacheCleaner *self();

  protected:
    void collectionExpired( const Collection &collection );
    int collectionScheduleInterval( const Collection &collection );
    bool hasChanged( const Collection &collection, const Collection &changed );
    bool shouldScheduleCollection( const Collection &collection );

  private:
    static CacheCleaner *sInstance;
};

} // namespace Server
} // namespace Akonadi

#endif
