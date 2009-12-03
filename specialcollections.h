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

#ifndef AKONADI_SPECIALCOLLECTIONS_H
#define AKONADI_SPECIALCOLLECTIONS_H

#include "akonadi_export.h"

#include <QtCore/QObject>

#include "akonadi/collection.h"

class KCoreConfigSkeleton;
class KJob;

namespace Akonadi {

class AgentInstance;
class SpecialCollectionsPrivate;

/**
  @short An interface to special collections.

  This class is the central interface to special collections like inbox or
  outbox in a mail resource or recent contacts in a contacts resource.
  The class is not meant to be used directly, but to inherit the a type
  specific special collections class from it (e.g. SpecialMailCollections).

  To check whether a special collection is available, simply use the hasCollection() and
  hasDefaultCollection() methods. Available special collections are accessible through
  the collection() and defaultCollection() methods.

  To create a special collection, use a SpecialCollectionsRequestJob.
  This will create the special collections you request and automatically
  register them with SpecialCollections, so that it now knows they are available.

  This class monitors all special collections known to it, and removes it
  from the known list if they are deleted. Note that this class does not
  automatically rebuild the collections that disappeared.

  The defaultCollectionsChanged() and collectionsChanged() signals are emitted when
  the special collections for a resource change (i.e. some became available or some
  become unavailable).

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_EXPORT SpecialCollections : public QObject
{
  Q_OBJECT

  public:
    /**
     * Destroys the special collections object.
     */
    ~SpecialCollections();

    /**
     * Returns whether the given agent @p instance has a special collection of
     * the given @p type.
     */
    bool hasCollection( const QByteArray &type, const AgentInstance &instance ) const;

    /**
     * Returns the special collection of the given @p type in the given agent
     * @p instance, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection collection( const QByteArray &type, const AgentInstance &instance ) const;

    /**
     * Registers the given @p collection as a special collection
     * with the given @p type.
     *
     * The collection must be owned by a valid resource.
     * Registering a new collection of a previously registered type forgets the
     * old collection.
     */
    bool registerCollection( const QByteArray &type, const Akonadi::Collection &collection );

    /**
     * Returns whether the default resource has a special collection of
     * the given @p type.
     */
    bool hasDefaultCollection( const QByteArray &type ) const;

    /**
     * Returns the special collection of given @p type in the default
     * resource, or an invalid collection if such a collection is unknown.
     */
    Akonadi::Collection defaultCollection( const QByteArray &type ) const;

  Q_SIGNALS:
    /**
     * Emitted when the special collections for a resource have been changed
     * (for example, some become available, or some become unavailable).
     *
     * @param instance The instance of the resource the collection belongs to.
     */
    void collectionsChanged( const Akonadi::AgentInstance &instance );

    /**
     * Emitted when the special collections for the default resource have
     * been changed (for example, some become available, or some become unavailable).
     */
    void defaultCollectionsChanged();

  protected:
    /**
     * Creates a new special collections object.
     *
     * @param config The configuration skeleton that provides the default resource id.
     * @param parent The parent object.
     */
    explicit SpecialCollections( KCoreConfigSkeleton *config, QObject *parent = 0 );

  private:
    //@cond PRIVATE
    friend class SpecialCollectionsRequestJob;
    friend class SpecialCollectionsRequestJobPrivate;
    friend class SpecialCollectionsPrivate;

#if 1 // TODO do this only if building tests:
    friend class SpecialMailCollectionsTesting;
    friend class LocalFoldersTest;
#endif

    SpecialCollectionsPrivate *const d;

    Q_PRIVATE_SLOT( d, void collectionRemoved( Akonadi::Collection ) )
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONS_H
