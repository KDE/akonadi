/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEM_P_H
#define AKONADI_ITEM_P_H

#include <QtCore/QDateTime>
#include <QtCore/QMap>

#include "entity_p.h"
#include "itempayloadinternals_p.h"

namespace Akonadi {

/**
 * @internal
 */
class ItemPrivate : public EntityPrivate
{
  public:
    ItemPrivate( Item::Id id = -1 )
      : EntityPrivate( id ),
        mPayload( 0 ),
        mRevision( -1 ),
        mModificationTime(),
        mFlagsOverwritten( false )
    {
    }

    ItemPrivate( const ItemPrivate &other )
      : EntityPrivate( other )
    {
      mFlags = other.mFlags;
      mRevision = other.mRevision;
      mModificationTime = other.mModificationTime;
      mMimeType = other.mMimeType;
      if ( other.mPayload )
        mPayload = other.mPayload->clone();
      else
        mPayload = 0;
      mAddedFlags = other.mAddedFlags;
      mDeletedFlags = other.mDeletedFlags;
      mFlagsOverwritten = other.mFlagsOverwritten;
    }

    ~ItemPrivate()
    {
      delete mPayload;
    }

    void resetChangeLog()
    {
      mFlagsOverwritten = false;
      mAddedFlags.clear();
      mDeletedFlags.clear();
    }

    EntityPrivate *clone() const
    {
      return new ItemPrivate( *this );
    }

    PayloadBase*  mPayload;
    Item::Flags mFlags;
    int mRevision;
    QDateTime mModificationTime;
    QString mMimeType;
    Item::Flags mAddedFlags;
    Item::Flags mDeletedFlags;
    bool mFlagsOverwritten;
};

}

#endif

