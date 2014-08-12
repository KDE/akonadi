/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTION_H
#define AKONADI_COLLECTION_H

#include "akonadicore_export.h"
#include "entity.h"

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

class QUrl;

namespace Akonadi {

class CachePolicy;
class CollectionPrivate;
class CollectionStatistics;

/**
 * @short Represents a collection of PIM items.
 *
 * This class represents a collection of PIM items, such as a folder on a mail- or
 * groupware-server.
 *
 * Collections are hierarchical, i.e., they may have a parent collection.
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * // fetching all collections recursive, starting at the root collection
 * CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
 * connect( job, SIGNAL(result(KJob*)), SLOT(fetchFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::fetchFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
 *
 *   const Collection::List collections = fetchJob->collections();
 *   foreach ( const Collection &collection, collections ) {
 *     qDebug() << "Name:" << collection.name();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT Collection : public Entity
{
public:
    /**
     * Describes a list of collections.
     */
    typedef QList<Collection> List;

    /**
     * Describes rights of a collection.
     */
    enum Right {
        ReadOnly = 0x0,                   ///< Can only read items or subcollection of this collection
        CanChangeItem = 0x1,              ///< Can change items in this collection
        CanCreateItem = 0x2,              ///< Can create new items in this collection
        CanDeleteItem = 0x4,              ///< Can delete items in this collection
        CanChangeCollection = 0x8,        ///< Can change this collection
        CanCreateCollection = 0x10,       ///< Can create new subcollections in this collection
        CanDeleteCollection = 0x20,       ///< Can delete this collection
        CanLinkItem = 0x40,               ///< Can create links to existing items in this virtual collection @since 4.4
        CanUnlinkItem = 0x80,             ///< Can remove links to items in this virtual collection @since 4.4
        AllRights = (CanChangeItem | CanCreateItem | CanDeleteItem |
                     CanChangeCollection | CanCreateCollection | CanDeleteCollection)  ///< Has all rights on this storage collection
    };
    Q_DECLARE_FLAGS(Rights, Right)

    /**
     * Creates an invalid collection.
     */
    Collection();

    /**
     * Create a new collection.
     *
     * @param id The unique identifier of the collection.
     */
    explicit Collection(Id id);

    /**
     * Destroys the collection.
     */
    ~Collection();

    /**
     * Creates a collection from an @p other collection.
     */
    Collection(const Collection &other);

    /**
     * Creates a collection from the given @p url.
     */
    static Collection fromUrl(const QUrl &url);

    /**
     * Returns the i18n'ed name of the collection.
     */
    QString name() const;

    /**
     * Returns the display name (EntityDisplayAttribute::displayName()) if set,
     * and Collection::name() otherwise. For human-readable strings this is preferred
     * over Collection::name().
     *
     * @since 4.11
     */
    QString displayName() const;

    /**
     * Sets the i18n'ed name of the collection.
     *
     * @param name The new collection name.
     */
    void setName(const QString &name);

    /**
     * Returns the rights the user has on the collection.
     */
    Rights rights() const;

    /**
     * Sets the @p rights the user has on the collection.
     */
    void setRights(Rights rights);

    /**
     * Returns a list of possible content mimetypes,
     * e.g. message/rfc822, x-akonadi/collection for a mail folder that
     * supports sub-folders.
    */
    QStringList contentMimeTypes() const;

    /**
     * Sets the list of possible content mime @p types.
     */
    void setContentMimeTypes(const QStringList &types);

    /**
     * Returns the identifier of the parent collection.
     * @deprecated Use parentCollection()
     */
    AKONADICORE_DEPRECATED Id parent() const;

    /**
     * Sets the identifier of the @p parent collection.
     * @param parent the parent identifier to set
     * @deprecated Use setParentCollection()
     */
    AKONADICORE_DEPRECATED void setParent(Id parent);

    /**
     * Sets the parent @p collection.
     * @param collection the parent collection to set
     * @deprecated Use setParentCollection()
     */
    AKONADICORE_DEPRECATED void setParent(const Collection &collection);

    /**
     * Returns the parent remote identifier.
     * @note This usually returns nothing for collections retrieved from the backend.
     * @deprecated Use parentCollection()
     */
    AKONADICORE_DEPRECATED QString parentRemoteId() const;

    /**
     * Sets the parent's remote @p identifier.
     * @param identifier the parent's remote identifier to set
     * @deprecated Use setParentCollection()
     */
    AKONADICORE_DEPRECATED void setParentRemoteId(const QString &identifier);

    /**
     * Returns the root collection.
     */
    static Collection root();

    /**
     * Returns the mimetype used for collections.
     */
    static QString mimeType();

    /**
     * Returns the mimetype used for virtual collections
     *
     * @since 4.11
     */
    static QString virtualMimeType();

    /**
     * Returns the identifier of the resource owning the collection.
     */
    QString resource() const;

    /**
     * Sets the @p identifier of the resource owning the collection.
     */
    void setResource(const QString &identifier);

    /**
     * Returns the cache policy of the collection.
     */
    CachePolicy cachePolicy() const;

    /**
     * Sets the cache @p policy of the collection.
     */
    void setCachePolicy(const CachePolicy &policy);

    /**
     * Returns the collection statistics of the collection.
     */
    CollectionStatistics statistics() const;

    /**
     * Sets the collection @p statistics for the collection.
     */
    void setStatistics(const CollectionStatistics &statistics);

    /**
     * Describes the type of url which is returned in url().
     *
     * @since 4.7
     */
    enum UrlType {
        UrlShort = 0,     ///< A short url which contains the identifier only (equivalent to url())
        UrlWithName = 1   ///< A url with identifier and name
    };

    /**
     * Returns the url of the collection.
     * @param type the type of url
     * @since 4.7
     */
    QUrl url(UrlType type = UrlShort) const;

    /**
     * Returns whether the collection is virtual, for example a search collection.
     *
     * @since 4.6
     */
    bool isVirtual() const;

    /**
     * Sets whether the collection is virtual or not.
     * Virtual collections can't be converted to non-virtual and vice versa.
     * @param isVirtual virtual collection if @c true, otherwise a normal collection
     * @since 4.10
     */
    void setVirtual(bool isVirtual);

    /**
     * Sets the collection's enabled state.
     *
     * Use this mechanism to set if a collection should be available
     * to the user or not.
     *
     * This can be used in conjunction with the local list preference for finer grained control
     * to define if a collection should be included depending on the purpose.
     *
     * For example: A collection is by default enabled, meaning it is displayed to the user, synchronized by the resource,
     * and indexed by the indexer. A disabled collection on the other hand is not displayed, sychronized or indexed.
     * The local list preference allows to locally override that default value for each purpose individually.
     *
     * The enabled state can be synchronized by backends.
     * E.g. an imap resource may synchronize this with the subscription state.
     *
     * @since 4.14
     * @see setLocalListPreference, setShouldList
     */
    void setEnabled(bool enabled);

    /**
     * Returns the collection's enabled state.
     * @since 4.14
     * @see localListPreference
     */
    bool enabled() const;

    /**
     * Describes the list preference value
     *
     * @since 4.14
     */
    enum ListPreference {
        ListEnabled,  ///< Enable collection for specified purpose
        ListDisabled, ///< Disable collectoin for specified purpose
        ListDefault   ///< Fallback to enabled state
    };

    /**
     * Describes the purpose of the listing
     *
     * @since 4.14
     */
    enum ListPurpose {
        ListSync,     ///< Listing for synchronization
        ListDisplay,  ///< Listing for display to the user
        ListIndex     ///< Listing for indexing the content
    };

    /**
     * Sets the local list preference for the specified purpose.
     *
     * The local list preference overrides the enabled state unless set to ListDefault.
     * In case of ListDefault the enabled state should be taken as fallback (shouldList() implements this logic).
     *
     * The default value is ListDefault.
     *
     * @since 4.14
     * @see shouldList, setEnabled
     */
    void setLocalListPreference(ListPurpose purpose, ListPreference preference);

    /**
     * Returns the local list preference for the specified purpose.
     * @since 4.14
     * @see setLocalListPreference
     */
    ListPreference localListPreference(ListPurpose purpose) const;

    /**
     * Returns whether the collection should be listed or not for the specified purpose
     * Takes enabled state and local preference into account.
     *
     * @since 4.14
     * @see setLocalListPreference, setEnabled
     */
    bool shouldList(ListPurpose purpose) const;

    /**
     * Sets wether the collection should be listed or not for the specified purpose.
     * Takes enabled state and local preference into account.
     *
     * Use this instead of sestEnabled and setLocalListPreference to automatically set
     * the right setting.
     *
     * @since 4.14
     * @see setLocalListPreference, setEnabled
     */
    void setShouldList(ListPurpose purpose, bool shouldList);

    /**
     * Sets a collection to be referenced.
     *
     * A referenced collection is temporarily shown and synchronized even when disabled.
     * A reference is only valid for the duration of a session, and is automatically removed afterwards.
     *
     * Referenced collections are only visible if explicitly monitored in the ETM.
     *
     * @since 4.14
     */
    void setReferenced(bool referenced);

    /**
     * Returns the referenced state of the collection.
     * @since 4.14
     */
    bool referenced() const;

private:
    AKONADI_DECLARE_PRIVATE(Collection)
    friend class CollectionFetchJob;
    friend class CollectionModifyJob;
};

}

AKONADICORE_EXPORT uint qHash(const Akonadi::Collection &collection);
/**
 * Allows to output a collection for debugging purposes.
 */
AKONADICORE_EXPORT QDebug operator<<(QDebug d, const Akonadi::Collection &collection);

Q_DECLARE_METATYPE(Akonadi::Collection)
Q_DECLARE_METATYPE(Akonadi::Collection::List)
Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::Collection::Rights)

#endif
