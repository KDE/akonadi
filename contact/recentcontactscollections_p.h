/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_RECENTCONTACTSCOLLECTIONS_P_H
#define AKONADI_RECENTCONTACTSCOLLECTIONS_P_H

#include "akonadi-contact_export.h"

#include "akonadi/specialcollections.h"

namespace Akonadi {

class RecentContactsCollectionsPrivate;

/**
 * @internal
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT RecentContactsCollections : public SpecialCollections
{
  Q_OBJECT

  public:
    /**
     * Returns the global RecentContactsCollections instance.
     */
    static RecentContactsCollections *self();

    /**
     * Returns whether the given agent @p instance has a recent contacts collection.
     */
    bool hasCollection( const AgentInstance &instance ) const;

    /**
     * Returns the recent contacts collection in the given agent
     * @p instance, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection collection( const AgentInstance &instance ) const;

    /**
     * Registers the given @p collection as a recent contacts collection.
     * @param collection the contacts collection to register
     * The collection must be owned by a valid resource.
     */
    bool registerCollection( const Akonadi::Collection &collection );

    /**
     * Returns whether the default resource has a recent contacts collection.
     */
    bool hasDefaultCollection() const;

    /**
     * Returns the recent contacts collection in the default
     * resource, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection defaultCollection() const;

  private:
    //@cond PRIVATE
    friend class RecentContactsCollectionsPrivate;

    RecentContactsCollections( RecentContactsCollectionsPrivate *dd );

    RecentContactsCollectionsPrivate *const d;
    //@endcond
};

}

#endif
