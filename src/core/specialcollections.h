/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"

#include <QObject>

#include <memory>

class KCoreConfigSkeleton;

namespace Akonadi
{
class AgentInstance;
class SpecialCollectionsPrivate;

/*!
  \brief An interface to special collections.

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

  \class Akonadi::SpecialCollections
  \inheaders Akonadi/SpecialCollections
  \inmodule AkonadiCore

  \author Constantin Berzan <exit3219@gmail.com>
  \since 4.4
*/
class AKONADICORE_EXPORT SpecialCollections : public QObject
{
    Q_OBJECT

public:
    /*!
     * Destroys the special collections object.
     */
    ~SpecialCollections() override;

    /*!
     * Returns whether the given agent \a instance has a special collection of
     * the given \a type.
     */
    [[nodiscard]] bool hasCollection(const QByteArray &type, const AgentInstance &instance) const;

    /*!
     * Returns the special collection of the given \a type in the given agent
     * \a instance, or an invalid collection if such a collection is unknown.
     */
    [[nodiscard]] Akonadi::Collection collection(const QByteArray &type, const AgentInstance &instance) const;

    /*!
     * Registers the given \a collection as a special collection
     * with the given \a type.
     * \a type the special type of \\ collection
     * \a collection the given collection to register
     * The collection must be owned by a valid resource.
     * Registering a new collection of a previously registered type forgets the
     * old collection.
     */
    bool registerCollection(const QByteArray &type, const Akonadi::Collection &collection);

    /*!
     * Unregisters the given \a collection as a special collection.
     * \a type the special type of \\ collection
     * \since 4.12
     */
    bool unregisterCollection(const Collection &collection);

    /*!
     * unsets the special collection attribute which marks \a collection as being a special
     * collection.
     * \since 4.12
     */
    static void unsetSpecialCollection(const Akonadi::Collection &collection);

    /*!
     * Sets the special collection attribute which marks \a collection as being a special
     * collection of type \a type.
     * This is typically used by configuration dialog for resources, when the user can choose
     * a specific special collection (ex: IMAP trash).
     *
     * \since 4.11
     */
    static void setSpecialCollectionType(const QByteArray &type, const Akonadi::Collection &collection);

    /*!
     * Returns whether the default resource has a special collection of
     * the given \a type.
     */
    [[nodiscard]] bool hasDefaultCollection(const QByteArray &type) const;

    /*!
     * Returns the special collection of given \a type in the default
     * resource, or an invalid collection if such a collection is unknown.
     */
    [[nodiscard]] Akonadi::Collection defaultCollection(const QByteArray &type) const;

    /*!
     * Returns whether the instanceIdentifier is a special agent that should not be deleted.
     */
    [[nodiscard]] bool isSpecialAgent(const QString &instanceIdentifier) const;

Q_SIGNALS:
    /*!
     * Emitted when the special collections for a resource have been changed
     * (for example, some become available, or some become unavailable).
     *
     * \a instance The instance of the resource the collection belongs to.
     */
    void collectionsChanged(const Akonadi::AgentInstance &instance);

    /*!
     * Emitted when the special collections for the default resource have
     * been changed (for example, some become available, or some become unavailable).
     */
    void defaultCollectionsChanged();

protected:
    /*!
     * Creates a new special collections object.
     *
     * \a config The configuration skeleton that provides the default resource id.
     * \a parent The parent object.
     */
    explicit SpecialCollections(KCoreConfigSkeleton *config, QObject *parent = nullptr);

private:
    friend class SpecialCollectionsRequestJob;
    friend class SpecialCollectionsRequestJobPrivate;
    friend class SpecialCollectionsPrivate;
#ifdef BUILD_TESTING
    friend class SpecialMailCollectionsTesting;
    friend class LocalFoldersTest;
#endif

    std::unique_ptr<SpecialCollectionsPrivate> const d;
};

} // namespace Akonadi
