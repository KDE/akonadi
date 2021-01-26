/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SPECIALCOLLECTIONS_H
#define AKONADI_SPECIALCOLLECTIONS_H

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"

#include <QObject>

class KCoreConfigSkeleton;

namespace Akonadi
{
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
class AKONADICORE_EXPORT SpecialCollections : public QObject
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
    Q_REQUIRED_RESULT bool hasCollection(const QByteArray &type, const AgentInstance &instance) const;

    /**
     * Returns the special collection of the given @p type in the given agent
     * @p instance, or an invalid collection if such a collection is unknown.
     */
    Q_REQUIRED_RESULT Akonadi::Collection collection(const QByteArray &type, const AgentInstance &instance) const;

    /**
     * Registers the given @p collection as a special collection
     * with the given @p type.
     * @param type the special type of @c collection
     * @param collection the given collection to register
     * The collection must be owned by a valid resource.
     * Registering a new collection of a previously registered type forgets the
     * old collection.
     */
    bool registerCollection(const QByteArray &type, const Akonadi::Collection &collection);

    /**
     * Unregisters the given @p collection as a special collection.
     * @param type the special type of @c collection
     * @since 4.12
     */
    bool unregisterCollection(const Collection &collection);

    /**
     * unsets the special collection attribute which marks @p collection as being a special
     * collection.
     * @since 4.12
     */
    static void unsetSpecialCollection(const Akonadi::Collection &collection);

    /**
     * Sets the special collection attribute which marks @p collection as being a special
     * collection of type @p type.
     * This is typically used by configuration dialog for resources, when the user can choose
     * a specific special collection (ex: IMAP trash).
     *
     * @since 4.11
     */
    static void setSpecialCollectionType(const QByteArray &type, const Akonadi::Collection &collection);

    /**
     * Returns whether the default resource has a special collection of
     * the given @p type.
     */
    Q_REQUIRED_RESULT bool hasDefaultCollection(const QByteArray &type) const;

    /**
     * Returns the special collection of given @p type in the default
     * resource, or an invalid collection if such a collection is unknown.
     */
    Q_REQUIRED_RESULT Akonadi::Collection defaultCollection(const QByteArray &type) const;

Q_SIGNALS:
    /**
     * Emitted when the special collections for a resource have been changed
     * (for example, some become available, or some become unavailable).
     *
     * @param instance The instance of the resource the collection belongs to.
     */
    void collectionsChanged(const Akonadi::AgentInstance &instance);

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
    explicit SpecialCollections(KCoreConfigSkeleton *config, QObject *parent = nullptr);

private:
    //@cond PRIVATE
    friend class SpecialCollectionsRequestJob;
    friend class SpecialCollectionsRequestJobPrivate;
    friend class SpecialCollectionsPrivate;
#ifdef BUILD_TESTING
    friend class SpecialMailCollectionsTesting;
    friend class LocalFoldersTest;
#endif

    SpecialCollectionsPrivate *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONS_H
