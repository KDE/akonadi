/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <QDebug>
#include <QMetaType>
#include <QSharedDataPointer>

class QUrl;

namespace Akonadi
{
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
 *   for ( const Collection &collection : collections ) {
 *     qDebug() << "Name:" << collection.name();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT Collection
{
    Q_GADGET
    Q_PROPERTY(Id id READ id WRITE setId)
    Q_PROPERTY(QString remoteIdd READ remoteId WRITE setRemoteId)
    Q_PROPERTY(bool isValid READ isValid)
    Q_PROPERTY(QString remoteRevision READ remoteRevision WRITE setRemoteRevision)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
    Q_PROPERTY(bool isVirtual READ isVirtual WRITE setVirtual)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString displayName READ displayName)
    Q_PROPERTY(Rights rights READ rights WRITE setRights)
    Q_PROPERTY(QStringList contentMimeTypes READ contentMimeTypes WRITE setContentMimeTypes)
    Q_PROPERTY(QString resource READ resource WRITE setResource)
public:
    /**
     * Describes the unique id type.
     */
    using Id = qint64;

    /**
     * Describes a list of collections.
     */
    using List = QVector<Collection>;

    /**
     * Describes rights of a collection.
     */
    enum Right {
        ReadOnly = 0x0, ///< Can only read items or subcollection of this collection
        CanChangeItem = 0x1, ///< Can change items in this collection
        CanCreateItem = 0x2, ///< Can create new items in this collection
        CanDeleteItem = 0x4, ///< Can delete items in this collection
        CanChangeCollection = 0x8, ///< Can change this collection
        CanCreateCollection = 0x10, ///< Can create new subcollections in this collection
        CanDeleteCollection = 0x20, ///< Can delete this collection
        CanLinkItem = 0x40, ///< Can create links to existing items in this virtual collection @since 4.4
        CanUnlinkItem = 0x80, ///< Can remove links to items in this virtual collection @since 4.4
        AllRights = (CanChangeItem | CanCreateItem | CanDeleteItem | CanChangeCollection | CanCreateCollection
                     | CanDeleteCollection) ///< Has all rights on this storage collection
    };
    Q_DECLARE_FLAGS(Rights, Right)
    Q_ENUM(Right)

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
     * Move constructor.
     */
    Collection(Collection &&other) noexcept;

    /**
     * Creates a collection from the given @p url.
     */
    static Collection fromUrl(const QUrl &url);

    /**
     * Sets the unique @p identifier of the collection.
     */
    void setId(Id identifier);

    /**
     * Returns the unique identifier of the collection.
     */
    Q_REQUIRED_RESULT Id id() const;

    /**
     * Sets the remote @p id of the collection.
     */
    void setRemoteId(const QString &id);

    /**
     * Returns the remote id of the collection.
     */
    Q_REQUIRED_RESULT QString remoteId() const;

    /**
     * Sets the remote @p revision of the collection.
     * @param revision the collections's remote revision
     * The remote revision can be used by resources to store some
     * revision information of the backend to detect changes there.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    void setRemoteRevision(const QString &revision);

    /**
     * Returns the remote revision of the collection.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    Q_REQUIRED_RESULT QString remoteRevision() const;

    /**
     * Returns whether the collection is valid.
     */
    Q_REQUIRED_RESULT bool isValid() const;

    /**
     * Returns whether this collections's id equals the
     * id of the @p other collection.
     */
    Q_REQUIRED_RESULT bool operator==(const Collection &other) const;

    /**
     * Returns whether the collection's id does not equal the id
     * of the @p other collection.
     */
    Q_REQUIRED_RESULT bool operator!=(const Collection &other) const;

    /**
     * Assigns the @p other to this collection and returns a reference to this
     * collection.
     * @param other the collection to assign
     */
    Collection &operator=(const Collection &other);

    /**
     * @internal For use with containers only.
     *
     * @since 4.8
     */
    Q_REQUIRED_RESULT bool operator<(const Collection &other) const;

    /**
     * Returns the parent collection of this object.
     * @note This will of course only return a useful value if it was explicitly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Q_REQUIRED_RESULT Collection parentCollection() const;

    /**
     * Returns a reference to the parent collection of this object.
     * @note This will of course only return a useful value if it was explicitly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Q_REQUIRED_RESULT Collection &parentCollection();

    /**
     * Set the parent collection of this object.
     * @note Calling this method has no immediate effect for the object itself,
     *       such as being moved to another collection.
     *       It is mainly relevant to provide a context for RID-based operations
     *       inside resources.
     * @param parent The parent collection.
     * @since 4.4
     */
    void setParentCollection(const Collection &parent);

    /**
     * Adds an attribute to the collection.
     *
     * If an attribute of the same type name already exists, it is deleted and
     * replaced with the new one.
     *
     * @param attribute The new attribute.
     *
     * @note The collection takes the ownership of the attribute.
     */
    void addAttribute(Attribute *attribute);

    /**
     * Removes and deletes the attribute of the given type @p name.
     */
    void removeAttribute(const QByteArray &name);

    /**
     * Returns @c true if the collection has an attribute of the given type @p name,
     * false otherwise.
     */
    bool hasAttribute(const QByteArray &name) const;

    /**
     * Returns a list of all attributes of the collection.
     *
     * @warning Do not modify the attributes returned from this method,
     * the change will not be reflected when updating the Collection
     * through CollectionModifyJob.
     */
    Q_REQUIRED_RESULT Attribute::List attributes() const;

    /**
     * Removes and deletes all attributes of the collection.
     */
    void clearAttributes();

    /**
     * Returns the attribute of the given type @p name if available, 0 otherwise.
     */
    Attribute *attribute(const QByteArray &name);
    const Attribute *attribute(const QByteArray &name) const;

    /**
     * Describes the options that can be passed to access attributes.
     */
    enum CreateOption {
        AddIfMissing, ///< Creates the attribute if it is missing
        DontCreate ///< Default value
    };

    /**
     * Returns the attribute of the requested type.
     * If the collection has no attribute of that type yet, passing AddIfMissing
     * as an argument will create and add it to the entity
     *
     * @param option The create options.
     */
    template<typename T>
    inline T *attribute(CreateOption option = DontCreate);

    /**
     * Returns the attribute of the requested type or 0 if it is not available.
     */
    template<typename T>
    inline const T *attribute() const;

    /**
     * Removes and deletes the attribute of the requested type.
     */
    template<typename T>
    inline void removeAttribute();

    /**
     * Returns whether the collection has an attribute of the requested type.
     */
    template<typename T>
    inline bool hasAttribute() const;

    /**
     * Returns the i18n'ed name of the collection.
     */
    Q_REQUIRED_RESULT QString name() const;

    /**
     * Returns the display name (EntityDisplayAttribute::displayName()) if set,
     * and Collection::name() otherwise. For human-readable strings this is preferred
     * over Collection::name().
     *
     * @since 4.11
     */
    Q_REQUIRED_RESULT QString displayName() const;

    /**
     * Sets the i18n'ed name of the collection.
     *
     * @param name The new collection name.
     */
    void setName(const QString &name);

    /**
     * Returns the rights the user has on the collection.
     */
    Q_REQUIRED_RESULT Rights rights() const;

    /**
     * Sets the @p rights the user has on the collection.
     */
    void setRights(Rights rights);

    /**
     * Returns a list of possible content mimetypes,
     * e.g. message/rfc822, x-akonadi/collection for a mail folder that
     * supports sub-folders.
     */
    Q_REQUIRED_RESULT QStringList contentMimeTypes() const;

    /**
     * Sets the list of possible content mime @p types.
     */
    void setContentMimeTypes(const QStringList &types);

    /**
     * Returns the root collection.
     */
    Q_REQUIRED_RESULT static Collection root();

    /**
     * Returns the mimetype used for collections.
     */
    Q_REQUIRED_RESULT static QString mimeType();

    /**
     * Returns the mimetype used for virtual collections
     *
     * @since 4.11
     */
    Q_REQUIRED_RESULT static QString virtualMimeType();

    /**
     * Returns the identifier of the resource owning the collection.
     */
    Q_REQUIRED_RESULT QString resource() const;

    /**
     * Sets the @p identifier of the resource owning the collection.
     */
    void setResource(const QString &identifier);

    /**
     * Returns the cache policy of the collection.
     */
    Q_REQUIRED_RESULT CachePolicy cachePolicy() const;

    /**
     * Sets the cache @p policy of the collection.
     */
    void setCachePolicy(const CachePolicy &policy);

    /**
     * Returns the collection statistics of the collection.
     */
    Q_REQUIRED_RESULT CollectionStatistics statistics() const;

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
        UrlShort = 0, ///< A short url which contains the identifier only (equivalent to url())
        UrlWithName = 1 ///< A url with identifier and name
    };

    /**
     * Returns the url of the collection.
     * @param type the type of url
     * @since 4.7
     */
    Q_REQUIRED_RESULT QUrl url(UrlType type = UrlShort) const;

    /**
     * Returns whether the collection is virtual, for example a search collection.
     *
     * @since 4.6
     */
    Q_REQUIRED_RESULT bool isVirtual() const;

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
     * and indexed by the indexer. A disabled collection on the other hand is not displayed, synchronized or indexed.
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
    Q_REQUIRED_RESULT bool enabled() const;

    /**
     * Describes the list preference value
     *
     * @since 4.14
     */
    enum ListPreference {
        ListEnabled, ///< Enable collection for specified purpose
        ListDisabled, ///< Disable collection for specified purpose
        ListDefault ///< Fallback to enabled state
    };

    /**
     * Describes the purpose of the listing
     *
     * @since 4.14
     */
    enum ListPurpose {
        ListSync, ///< Listing for synchronization
        ListDisplay, ///< Listing for display to the user
        ListIndex ///< Listing for indexing the content
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
    Q_REQUIRED_RESULT ListPreference localListPreference(ListPurpose purpose) const;

    /**
     * Returns whether the collection should be listed or not for the specified purpose
     * Takes enabled state and local preference into account.
     *
     * @since 4.14
     * @see setLocalListPreference, setEnabled
     */
    Q_REQUIRED_RESULT bool shouldList(ListPurpose purpose) const;

    /**
     * Sets whether the collection should be listed or not for the specified purpose.
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
     * Set during sync to indicate that the provided parts are only default values;
     * @since 4.15
     */
    void setKeepLocalChanges(const QSet<QByteArray> &parts);

    /**
     * Returns what parts are only default values.
     */
    QSet<QByteArray> keepLocalChanges() const;

private:
    friend class CollectionCreateJob;
    friend class CollectionFetchJob;
    friend class CollectionModifyJob;
    friend class ProtocolHelper;

    void markAttributeModified(const QByteArray &type);

    /// @cond PRIVATE
    QSharedDataPointer<CollectionPrivate> d_ptr;
    friend class CollectionPrivate;
    /// @endcond
};

AKONADICORE_EXPORT uint qHash(const Akonadi::Collection &collection);

template<typename T>
inline T *Akonadi::Collection::attribute(Collection::CreateOption option)
{
    const QByteArray type = T().type();
    markAttributeModified(type); // do this first in case it detaches
    if (hasAttribute(type)) {
        if (T *attr = dynamic_cast<T *>(attribute(type))) {
            return attr;
        }
        qWarning() << "Found attribute of unknown type" << type << ". Did you forget to call AttributeFactory::registerAttribute()?";
    } else if (option == AddIfMissing) {
        T *attr = new T();
        addAttribute(attr);
        return attr;
    }

    return nullptr;
}

template<typename T>
inline const T *Akonadi::Collection::attribute() const
{
    const QByteArray type = T().type();
    if (hasAttribute(type)) {
        if (const T *attr = dynamic_cast<const T *>(attribute(type))) {
            return attr;
        }
        qWarning() << "Found attribute of unknown type" << type << ". Did you forget to call AttributeFactory::registerAttribute()?";
    }

    return nullptr;
}

template<typename T>
inline void Akonadi::Collection::removeAttribute()
{
    removeAttribute(T().type());
}

template<typename T>
inline bool Akonadi::Collection::hasAttribute() const
{
    return hasAttribute(T().type());
}

} // namespace Akonadi

/**
 * Allows to output a collection for debugging purposes.
 */
AKONADICORE_EXPORT QDebug operator<<(QDebug d, const Akonadi::Collection &collection);

Q_DECLARE_METATYPE(Akonadi::Collection)
Q_DECLARE_METATYPE(Akonadi::Collection::List)
Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::Collection::Rights)
Q_DECLARE_TYPEINFO(Akonadi::Collection, Q_MOVABLE_TYPE);
