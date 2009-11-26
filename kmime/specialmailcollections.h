/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_SPECIALMAILCOLLECTIONS_H
#define AKONADI_SPECIALMAILCOLLECTIONS_H

#include "akonadi-kmime_export.h"

#include "akonadi/specialcollections.h"

namespace Akonadi {

class SpecialMailCollectionsPrivate;

/**
  @short Interface to special mail collections such as inbox, outbox etc.

  This class is the central interface to the local mail folders. These folders
  can either be in the default resource (stored in ~/.local/share/local-mail)
  or in any number of custom resources. Special collections of the following types
  are supported: inbox, outbox, sent-mail, trash, drafts, templates and spam.

  To check whether a special mail collection is available, simply use the hasCollection() and
  hasDefaultCollection() methods. Available special mail collections are accessible through
  the collection() and defaultCollection() methods.

  To create a special mail collection, use a SpecialMailCollectionsRequestJob.
  This will create the special mail collections you request and automatically
  register them with SpecialMailCollections, so that it now knows they are available.

  This class monitors all special mail collections known to it, and removes it
  from the known list if they are deleted. Note that this class does not
  automatically rebuild the collections that disappeared.

  The defaultCollectionsChanged() and collectionsChanged() signals are emitted when
  the special mail collections for a resource change (i.e. some became available or some
  become unavailable).

  @code
  if( SpecialMailCollections::self()->hasDefaultCollection( SpecialMailCollections::Outbox ) ) {
    const Collection col = SpecialMailCollections::self()->defaultCollection( SpecialMailCollections::Outbox );
    // ...
  } else {
    // ... use SpecialMailCollectionsRequestJob to request the collection...
  }
  @endcode

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT SpecialMailCollections : public SpecialCollections
{
  Q_OBJECT

  public:
    /**
     * Describes the possible types of special mail collections.
     *
     * Generally, there may not be two special mail collections of
     * the same type in the same resource.
     */
    enum Type
    {
      Invalid = -1,    ///< An invalid special collection.
      Root = 0,        ///< The root collection containing the local folders.
      Inbox,           ///< The inbox collection.
      Outbox,          ///< The outbox collection.
      SentMail,        ///< The sent-mail collection.
      Trash,           ///< The trash collection.
      Drafts,          ///< The drafts collection.
      Templates,       ///< The templates collection.
      LastType         ///< @internal marker
    };

    /**
     * Returns the global SpecialMailCollections instance.
     */
    static SpecialMailCollections *self();

    /**
     * Returns whether the given agent @p instance has a special collection of
     * the given @p type.
     */
    bool hasCollection( Type type, const AgentInstance &instance ) const;

    /**
     * Returns the special mail collection of the given @p type in the given agent
     * @p instance, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection collection( Type type, const AgentInstance &instance ) const;

    /**
     * Registers the given @p collection as a special mail collection
     * with the given @p type.
     *
     * The collection must be owned by a valid resource.
     * Registering a new collection of a previously registered type forgets the
     * old collection.
     */
    bool registerCollection( Type type, const Akonadi::Collection &collection );

    /**
     * Returns whether the default resource has a special mail collection of
     * the given @p type.
     */
    bool hasDefaultCollection( Type type ) const;

    /**
     * Returns the special mail collection of given @p type in the default
     * resource, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection defaultCollection( Type type ) const;

  private:
    //@cond PRIVATE
    friend class SpecialMailCollectionsPrivate;

#if 1 // TODO do this only if building tests:
    friend class SpecialMailCollectionsTesting;
    friend class LocalFoldersTest;
#endif

    SpecialMailCollections( SpecialMailCollectionsPrivate *dd );

    SpecialMailCollectionsPrivate *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALMAILCOLLECTIONS_H
