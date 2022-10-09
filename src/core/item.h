/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>
                  2007 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"
#include "collection.h"
#include "exceptionbase.h"
#include "itempayloadinternals_p.h"
#include "job.h"
#include "relation.h"
#include "tag.h"

#include <QByteArray>
#include <QMetaType>
#include <QSet>

#include <memory>
#include <type_traits>
#include <typeinfo>

class QUrl;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
template<typename T>
class QVector;
#else
template<typename T>
class QList;
#endif

namespace Akonadi
{
class ItemPrivate;

/**
 * @short Represents a PIM item stored in Akonadi storage.
 *
 * A PIM item consists of one or more parts, allowing a fine-grained access on its
 * content where needed (eg. mail envelope, mail body and attachments).
 *
 * There is also a namespace (prefix) for special parts which are local to Akonadi.
 * These parts, prefixed by "akonadi-", will never be fetched in the resource.
 * They are useful for local extensions like agents which might want to add meta data
 * to items in order to handle them but the meta data should not be stored back to the
 * resource.
 *
 * This class is implicitly shared.
 *
 * <h4>Payload</h4>
 *
 * This class contains, beside some type-agnostic information (flags, revision),
 * zero or more payload objects representing its actual data. Which objects these actually
 * are depends on the mimetype of the item and the corresponding serializer plugin(s).
 *
 * Technically the only restriction on payload objects is that they have to be copyable.
 * For safety reasons, pointer payloads are forbidden as well though, as the
 * ownership would not be clear. In this case, usage of a shared pointer is
 * recommended (such as QSharedPointer or std::shared_ptr).
 *
 * Using a shared pointer is also required in case the payload is a polymorphic
 * type. For supported shared pointer types implicit casting is provided when possible.
 *
 * When using a value-based class as payload, it is recommended to use one that does
 * support implicit sharing as setting and retrieving a payload as well as copying
 * an Akonadi::Item object imply copying of the payload object.
 *
 * Since KDE 4.6, Item supports multiple payload types per mime type,
 * and will automatically convert between them using the serialiser
 * plugins (which is slow). It also supports mixing shared pointer
 * types, e.g. inserting a std::shared_ptr<Foo> and extracting a
 * QSharedPointer<Foo>. Since the two shared pointer types cannot
 * share ownership of the same object, the payload class @c T needs to
 * provide a @c clone() method with the usual signature, ie.
 *
 * @code
 * virtual T * T::clone() const
 * @endcode
 *
 * If the class that does not have a @c clone() method, asking for an
 * incompatible shared pointer will throw a PayloadException.
 *
 * Since using different shared pointer types and different payload
 * types for the same mimetype incurs slow conversions (between
 * payload types) and cloning (between shared pointer types), as well
 * as manifold memory usage (results of conversions are cached inside
 * the Item, and only destroyed when a new payload is set by the user
 * of the class), you want to restrict yourself to just one type and
 * one shared pointer type. This mechanism was mainly introduced for
 * backwards compatibility (e.g., putting in a
 * std::shared_ptr<KCal::Incidence> and extracting a
 * QSharedPointer<KCalCore::Incidence>), so it is not optimized for
 * performance.
 *
 * The availability of a payload of a specific type can be checked using hasPayload(),
 * payloads can be retrieved by using payload() and set by using setPayload(). Refer
 * to the documentation of those methods for more details.
 *
 * @author Volker Krause <vkrause@kde.org>, Till Adam <adam@kde.org>, Marc Mutz <mutz@kde.org>
 */
class AKONADICORE_EXPORT Item
{
public:
    /**
     * Describes the unique id type.
     */
    using Id = qint64;

    /**
     * Describes a list of items.
     */
    using List = QVector<Item>;

    /**
     * Describes a flag name.
     */
    using Flag = QByteArray;

    /**
     * Describes a set of flag names.
     */
    using Flags = QSet<QByteArray>;

    /**
     * Describes the part name that is used to fetch the
     * full payload of an item.
     */
    static const char FullPayload[];

    /**
     * Creates a new item.
     */
    Item();

    /**
     * Creates a new item with the given unique @p id.
     */
    explicit Item(Id id);

    /**
     * Creates a new item with the given mime type.
     *
     * @param mimeType The mime type of the item.
     */
    explicit Item(const QString &mimeType);

    /**
     * Creates a new item from an @p other item.
     */
    Item(const Item &other);

    /**
     * Move constructor.
     */
    Item(Item &&) noexcept;

    /**
     * Destroys the item.
     */
    ~Item();

    /**
     * Creates an item from the given @p url.
     */
    static Item fromUrl(const QUrl &url);

    /**
     * Sets the unique @p identifier of the item.
     */
    void setId(Id identifier);

    /**
     * Returns the unique identifier of the item.
     */
    Id id() const;

    /**
     * Sets the remote @p id of the item.
     */
    void setRemoteId(const QString &id);

    /**
     * Returns the remote id of the item.
     */
    QString remoteId() const;

    /**
     * Sets the remote @p revision of the item.
     * @param revision the item's remote revision
     * The remote revision can be used by resources to store some
     * revision information of the backend to detect changes there.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    void setRemoteRevision(const QString &revision);

    /**
     * Returns the remote revision of the item.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    QString remoteRevision() const;

    /**
     * Returns whether the item is valid.
     */
    bool isValid() const;

    /**
     * Returns whether this item's id equals the id of the @p other item.
     */
    bool operator==(const Item &other) const;

    /**
     * Returns whether the item's id does not equal the id of the @p other item.
     */
    bool operator!=(const Item &other) const;

    /**
     * Assigns the @p other to this item and returns a reference to this item.
     * @param other the item to assign
     */
    Item &operator=(const Item &other);

    /**
     * @internal For use with containers only.
     *
     * @since 4.8
     */
    bool operator<(const Item &other) const;

    /**
     * Returns the parent collection of this object.
     * @note This will of course only return a useful value if it was explicitly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Collection parentCollection() const;

    /**
     * Returns a reference to the parent collection of this object.
     * @note This will of course only return a useful value if it was explicitly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Collection &parentCollection();

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
     * Adds an attribute to the item.
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
     * Returns @c true if the item has an attribute of the given type @p name,
     * false otherwise.
     */
    bool hasAttribute(const QByteArray &name) const;

    /**
     * Returns a list of all attributes of the item.
     *
     * @warning Do not modify the attributes returned from this method,
     * the change will not be reflected when updating the Item through
     * ItemModifyJob.
     */
    Attribute::List attributes() const;

    /**
     * Removes and deletes all attributes of the item.
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
        DontCreate ///< Do not create the attribute if it is missing (default)
    };

    /**
     * Returns the attribute of the requested type.
     * If the item has no attribute of that type yet, a new one
     * is created and added to the entity.
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
     * Returns whether the item has an attribute of the requested type.
     */
    template<typename T>
    inline bool hasAttribute() const;

    /**
     * Returns all flags of this item.
     */
    Flags flags() const;

    /**
     * Returns the timestamp of the last modification of this item.
     * @since 4.2
     */
    QDateTime modificationTime() const;

    /**
     * Sets the timestamp of the last modification of this item.
     * @param datetime the modification time to set
     * @note Do not modify this value from within an application,
     * it is updated automatically by the revision checking functions.
     * @since 4.2
     */
    void setModificationTime(const QDateTime &datetime);

    /**
     * Returns whether the flag with the given @p name is
     * set in the item.
     */
    bool hasFlag(const QByteArray &name) const;

    /**
     * Sets the flag with the given @p name in the item.
     */
    void setFlag(const QByteArray &name);

    /**
     * Removes the flag with the given @p name from the item.
     */
    void clearFlag(const QByteArray &name);

    /**
     * Overwrites all flags of the item by the given @p flags.
     */
    void setFlags(const Flags &flags);

    /**
     * Removes all flags from the item.
     */
    void clearFlags();

    void setTags(const Tag::List &list);

    void setTag(const Tag &tag);

    Tag::List tags() const;

    bool hasTag(const Tag &tag) const;

    void clearTag(const Tag &tag);

    void clearTags();

    /**
     * Returns all relations of this item.
     * @since 4.15
     * @see RelationCreateJob, RelationDeleteJob to modify relations
     */
    Relation::List relations() const;

    /**
     * Sets the payload based on the canonical representation normally
     * used for data of this mime type.
     *
     * @param data The encoded data.
     * @see fullPayloadData
     */
    void setPayloadFromData(const QByteArray &data);

    /**
     * Returns the full payload in its canonical representation, e.g. the
     * binary or textual format usually used for data with this mime type.
     * This is useful when communicating with non-Akonadi application by
     * e.g. drag&drop, copy&paste or stored files.
     */
    QByteArray payloadData() const;

    /**
     * Returns the list of loaded payload parts. This is not necessarily
     * identical to all parts in the cache or to all available parts on the backend.
     */
    QSet<QByteArray> loadedPayloadParts() const;

    /**
     * Marks that the payload shall be cleared from the cache when this
     * item is passed to an ItemModifyJob the next time.
     * This will trigger a refetch of the payload from the backend when the
     * item is accessed afterwards. Only resources should have a need for
     * this functionality.
     *
     * @since 4.5
     */
    void clearPayload();

    /**
     * Sets the @p revision number of the item.
     * @param revision the revision number to set
     * @note Do not modify this value from within an application,
     * it is updated automatically by the revision checking functions.
     */
    void setRevision(int revision);

    /**
     * Returns the revision number of the item.
     */
    int revision() const;

    /**
     * Returns the unique identifier of the collection this item is stored in. There is only
     * a single such collection, although the item can be linked into arbitrary many
     * virtual collections.
     * Calling this method makes sense only after running an ItemFetchJob on the item.
     * @returns the collection ID if it is known, -1 otherwise.
     * @since 4.3
     */
    Collection::Id storageCollectionId() const;

    /**
     * Set the size of the item in bytes.
     * @param size the size of the item in bytes
     * @since 4.2
     */
    void setSize(qint64 size);

    /**
     * Returns the size of the items in bytes.
     *
     * @since 4.2
     */
    qint64 size() const;

    /**
     * Sets the mime type of the item to @p mimeType.
     */
    void setMimeType(const QString &mimeType);

    /**
     * Returns the mime type of the item.
     */
    QString mimeType() const;

    /**
     * Sets the @p gid of the entity.
     *
     * @since 4.12
     */
    void setGid(const QString &gid);

    /**
     * Returns the gid of the entity.
     *
     * @since 4.12
     */
    QString gid() const;

    /**
     * Sets the virtual @p collections that this item is linked into.
     *
     * @note Note that changing this value makes no effect on what collections
     * this item is linked to. To link or unlink an item to/from a virtual
     * collection, use LinkJob and UnlinkJob.
     *
     * @since 4.14
     */
    void setVirtualReferences(const Collection::List &collections);

    /**
     * Lists virtual collections that this item is linked to.
     *
     * @note This value is populated only when this item was retrieved by
     * ItemFetchJob with fetchVirtualReferences set to true in ItemFetchScope,
     * otherwise this list is always empty.
     *
     * @since 4.14
     */
    Collection::List virtualReferences() const;

    /**
     * Returns a list of metatype-ids, describing the different
     * variants of payload that are currently contained in this item.
     *
     * The result is always sorted (increasing ids).
     */
    QVector<int> availablePayloadMetaTypeIds() const;

    /**
     * Sets a path to a file with full payload.
     *
     * This method can only be used by Resources and should not be used by Akonadi
     * clients. Clients should use setPayload() instead.
     *
     * Akonadi will not duplicate content of the file in its database but will
     * instead directly refer to this file. This means that the file must be
     * persistent (don't use this method with a temporary files), and the Akonadi
     * resource that owns the storage is responsible for updating the file path
     * if the file is changed, moved or removed.
     *
     * The payload can still be accessed via payload() methods.
     *
     * @see setPayload(), setPayloadFromData()
     * @since 5.6
     */
    void setPayloadPath(const QString &filePath);

    /**
     * Returns path to the payload file set by setPayloadPath()
     *
     * If payload was set via setPayload() or setPayloadFromData() then this
     * method will return a null string.
     */
    QString payloadPath() const;

    /**
     * Sets the payload object of this PIM item.
     *
     * @param p The payload object. Must be copyable and must not be a pointer,
     * will cause a compilation failure otherwise. Using a type that can be copied
     * fast (such as implicitly shared classes) is recommended.
     * If the payload type is polymorphic and you intend to set and retrieve payload
     * objects with mismatching but castable types, make sure to use a supported
     * shared pointer implementation (currently QSharedPointer and std::shared_ptr)
     * and make sure there is a specialization of Akonadi::super_trait for your class.
     */
    template<typename T>
    void setPayload(const T &p);
    /// @cond PRIVATE
    template<typename T>
    void setPayload(T *p);
    /// @endcond

    /**
     * Returns the payload object of this PIM item. This method will only succeed if either
     * you requested the exact same payload type that was put in or the payload uses a
     * supported shared pointer type (currently QSharedPointer and std::shared_ptr),
     * and is castable to the requested type. For this to work there needs
     * to be a specialization of Akonadi::super_trait of the used classes.
     *
     * If a mismatching or non-castable payload type is requested, an Akonadi::PayloadException
     * is thrown. Therefore it is generally recommended to guard calls to payload() with a
     * corresponding hasPayload() call.
     *
     * Trying to retrieve a pointer type will fail to compile.
     */
    template<typename T>
    T payload() const;

    /**
     * Returns whether the item has a payload object.
     */
    bool hasPayload() const;

    /**
     * Returns whether the item has a payload of type @c T.
     * This method will only return @c true if either you requested the exact same payload type
     * that was put in or the payload uses a supported shared pointer type (currently
     * QSharedPointer and std::shared_ptr), and is castable to the requested type. For this to work there needs
     * to be a specialization of Akonadi::super_trait of the used classes.
     *
     * Trying to retrieve a pointer type will fail to compile.
     */
    template<typename T>
    bool hasPayload() const;

    /**
     * Describes the type of url which is returned in url().
     */
    enum UrlType {
        UrlShort = 0, ///< A short url which contains the identifier only (default)
        UrlWithMimeType = 1 ///< A url with identifier and mimetype
    };

    /**
     * Returns the url of the item.
     */
    QUrl url(UrlType type = UrlShort) const;

    /**
     * Returns the parts available for this item.
     *
     * The returned set refers to parts available on the akonadi server or remotely,
     * but does not include the loadedPayloadParts() of this item.
     *
     * @since 4.4
     */
    QSet<QByteArray> availablePayloadParts() const;

    /**
     * Returns the parts available for this item in the cache. The list might be a subset
     * of the actual parts in cache, as it contains only the requested parts. See @see ItemFetchJob and
     * @see ItemFetchScope
     *
     * The returned set refers to parts available on the akonadi server.
     *
     * @since 4.11
     */
    QSet<QByteArray> cachedPayloadParts() const;

    /**
     * Applies the parts of Item @p other to this item.
     * Any parts or attributes available in other, will be applied to this item,
     * and the payload parts of other will be inserted into this item, overwriting
     * any existing parts with the same part name.
     *
     * If there is an ItemSerialzerPluginV2 for the type, the merge method in that plugin is
     * used to perform the merge. If only an ItemSerialzerPlugin class is found, or the merge
     * method of the -V2 plugin is not implemented, the merge is performed with multiple deserializations
     * of the payload.
     * @param other the item to get values from
     * @since 4.4
     */
    void apply(const Item &other);

    void setCachedPayloadParts(const QSet<QByteArray> &cachedParts);

private:
    /// @cond PRIVATE
    friend class ItemCreateJob;
    friend class ItemCreateJobPrivate;
    friend class ItemModifyJob;
    friend class ItemModifyJobPrivate;
    friend class ItemSync;
    friend class ProtocolHelper;
    Internal::PayloadBase *payloadBaseV2(int sharedPointerId, int metaTypeId) const;
    void setPayloadBaseV2(int sharedPointerId, int metaTypeId, std::unique_ptr<Internal::PayloadBase> &p);
    void addPayloadBaseVariant(int sharedPointerId, int metaTypeId, std::unique_ptr<Internal::PayloadBase> &p) const;

    /**
     * Try to ensure that we have a variant of the payload for metatype id @a mtid.
     * @return @c true if a type exists or could be created through conversion, @c false otherwise.
     */
    bool ensureMetaTypeId(int mtid) const;

    template<typename T>
    typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic, void>::type setPayloadImpl(const T &p, const int * /*disambiguate*/ = nullptr);
    template<typename T>
    typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic, void>::type setPayloadImpl(const T &p);

    template<typename T>
    typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic, T>::type payloadImpl(const int * /*disambiguate*/ = nullptr) const;
    template<typename T>
    typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic, T>::type payloadImpl() const;

    template<typename T>
    typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic, bool>::type hasPayloadImpl(const int * /*disambiguate*/ = nullptr) const;
    template<typename T>
    typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic, bool>::type hasPayloadImpl() const;

    template<typename T>
    typename std::enable_if<Internal::is_shared_pointer<T>::value, bool>::type tryToClone(T *ret, const int * /*disambiguate*/ = nullptr) const;
    template<typename T>
    typename std::enable_if<!Internal::is_shared_pointer<T>::value, bool>::type tryToClone(T *ret) const;

    template<typename T, typename NewT>
    typename std::enable_if<!std::is_same<T, NewT>::value, bool>::type tryToCloneImpl(T *ret, const int * /*disambiguate*/ = nullptr) const;
    template<typename T, typename NewT>
    typename std::enable_if<std::is_same<T, NewT>::value, bool>::type tryToCloneImpl(T *ret) const;

    /**
     * Set the collection ID to where the item is stored in. Should be set only by the ItemFetchJob.
     * @param collectionId the unique identifier of the collection where this item is stored in.
     * @since 4.3
     */
    void setStorageCollectionId(Collection::Id collectionId);

#if 0
    /**
     * Helper function for non-template throwing of PayloadException.
     */
    QString payloadExceptionText(int spid, int mtid) const;

    /**
     * Non-template throwing of PayloadException.
     * Needs to be inline, otherwise catch (Akonadi::PayloadException)
     * won't work (only catch (Akonadi::Exception))
     */
    inline void throwPayloadException(int spid, int mtid) const
    {
        throw PayloadException(payloadExceptionText(spid, mtid));
    }
#else
    void throwPayloadException(int spid, int mtid) const;
#endif

    QSharedDataPointer<ItemPrivate> d_ptr;
    friend class ItemPrivate;
    /// @endcond
};

AKONADICORE_EXPORT uint qHash(const Akonadi::Item &item);

template<typename T>
inline T *Item::attribute(Item::CreateOption option)
{
    const QByteArray type = T().type();
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
inline const T *Item::attribute() const
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
inline void Item::removeAttribute()
{
    removeAttribute(T().type());
}

template<typename T>
inline bool Item::hasAttribute() const
{
    return hasAttribute(T().type());
}

template<typename T>
T Item::payload() const
{
    static_assert(!std::is_pointer<T>::value, "Payload must not be a pointer");

    if (!hasPayload()) {
        throwPayloadException(-1, -1);
    }

    return payloadImpl<T>();
}

template<typename T>
typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic, T>::type Item::payloadImpl(const int *) const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(PayloadType::isPolymorphic, "Non-polymorphic payload type in polymorphic implementation is not allowed");

    using Root_T = typename Internal::get_hierarchy_root<T>::type;
    using RootType = Internal::PayloadTrait<Root_T>;
    static_assert(!RootType::isPolymorphic,
                  "Root type of payload type must not be polymorphic"); // prevent endless recursion

    return PayloadType::castFrom(payloadImpl<Root_T>());
}

template<typename T>
typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic, T>::type Item::payloadImpl() const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(!PayloadType::isPolymorphic, "Polymorphic payload type in non-polymorphic implementation is not allowed");

    const int metaTypeId = PayloadType::elementMetaTypeId();

    // make sure that we have a payload format represented by 'metaTypeId':
    if (!ensureMetaTypeId(metaTypeId)) {
        throwPayloadException(PayloadType::sharedPointerId, metaTypeId);
    }

    // Check whether we have the exact payload
    // (metatype id and shared pointer type match)
    if (const Internal::Payload<T> *const p = Internal::payload_cast<T>(payloadBaseV2(PayloadType::sharedPointerId, metaTypeId))) {
        return p->payload;
    }

    T ret;
    if (!tryToClone<T>(&ret)) {
        throwPayloadException(PayloadType::sharedPointerId, metaTypeId);
    }
    return ret;
}

template<typename T, typename NewT>
typename std::enable_if<!std::is_same<T, NewT>::value, bool>::type Item::tryToCloneImpl(T *ret, const int *) const
{
    using PayloadType = Internal::PayloadTrait<T>;
    using NewPayloadType = Internal::PayloadTrait<NewT>;

    const int metaTypeId = PayloadType::elementMetaTypeId();
    Internal::PayloadBase *payloadBase = payloadBaseV2(NewPayloadType::sharedPointerId, metaTypeId);
    if (const Internal::Payload<NewT> *const p = Internal::payload_cast<NewT>(payloadBase)) {
        // If found, attempt to make a clone (required the payload to provide virtual T * T::clone() const)
        const T nt = PayloadType::clone(p->payload);
        if (!PayloadType::isNull(nt)) {
            // if clone succeeded, add the clone to the Item:
            std::unique_ptr<Internal::PayloadBase> npb(new Internal::Payload<T>(nt));
            addPayloadBaseVariant(PayloadType::sharedPointerId, metaTypeId, npb);
            // and return it
            if (ret) {
                *ret = nt;
            }
            return true;
        }
    }

    return tryToCloneImpl<T, typename Internal::shared_pointer_traits<NewT>::next_shared_ptr>(ret);
}

template<typename T, typename NewT>
typename std::enable_if<std::is_same<T, NewT>::value, bool>::type Item::tryToCloneImpl(T *) const
{
    return false;
}

template<typename T>
typename std::enable_if<Internal::is_shared_pointer<T>::value, bool>::type Item::tryToClone(T *ret, const int *) const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(!PayloadType::isPolymorphic, "Polymorphic payload type in non-polymorphic implementation is not allowed");

    return tryToCloneImpl<T, typename Internal::shared_pointer_traits<T>::next_shared_ptr>(ret);
}

template<typename T>
typename std::enable_if<!Internal::is_shared_pointer<T>::value, bool>::type Item::tryToClone(T *) const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(!PayloadType::isPolymorphic, "Polymorphic payload type in non-polymorphic implementation is not allowed");

    return false;
}

template<typename T>
bool Item::hasPayload() const
{
    static_assert(!std::is_pointer<T>::value, "Payload type cannot be a pointer");
    return hasPayload() && hasPayloadImpl<T>();
}

template<typename T>
typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic, bool>::type Item::hasPayloadImpl(const int *) const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(PayloadType::isPolymorphic, "Non-polymorphic payload type in polymorphic implementation is no allowed");

    using Root_T = typename Internal::get_hierarchy_root<T>::type;
    using RootType = Internal::PayloadTrait<Root_T>;
    static_assert(!RootType::isPolymorphic,
                  "Root type of payload type must not be polymorphic"); // prevent endless recursion

    try {
        return hasPayloadImpl<Root_T>() && PayloadType::canCastFrom(payload<Root_T>());
    } catch (const Akonadi::PayloadException &e) {
        qDebug() << e.what();
        Q_UNUSED(e)
        return false;
    }
}

template<typename T>
typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic, bool>::type Item::hasPayloadImpl() const
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(!PayloadType::isPolymorphic, "Polymorphic payload type in non-polymorphic implementation is not allowed");

    const int metaTypeId = PayloadType::elementMetaTypeId();

    // make sure that we have a payload format represented by 'metaTypeId':
    if (!ensureMetaTypeId(metaTypeId)) {
        return false;
    }

    // Check whether we have the exact payload
    // (metatype id and shared pointer type match)
    if (const Internal::Payload<T> *const p = Internal::payload_cast<T>(payloadBaseV2(PayloadType::sharedPointerId, metaTypeId))) {
        return true;
    }

    return tryToClone<T>(nullptr);
}

template<typename T>
void Item::setPayload(const T &p)
{
    static_assert(!std::is_pointer<T>::value, "Payload type must not be a pointer");
    setPayloadImpl(p);
}

template<typename T>
typename std::enable_if<Internal::PayloadTrait<T>::isPolymorphic>::type Item::setPayloadImpl(const T &p, const int *)
{
    using PayloadType = Internal::PayloadTrait<T>;
    static_assert(PayloadType::isPolymorphic, "Non-polymorphic payload type in polymorphic implementation is not allowed");

    using Root_T = typename Internal::get_hierarchy_root<T>::type;
    using RootType = Internal::PayloadTrait<Root_T>;
    static_assert(!RootType::isPolymorphic,
                  "Root type of payload type must not be polymorphic"); // prevent endless recursion

    setPayloadImpl<Root_T>(p);
}

template<typename T>
typename std::enable_if<!Internal::PayloadTrait<T>::isPolymorphic>::type Item::setPayloadImpl(const T &p)
{
    using PayloadType = Internal::PayloadTrait<T>;
    std::unique_ptr<Internal::PayloadBase> pb(new Internal::Payload<T>(p));
    setPayloadBaseV2(PayloadType::sharedPointerId, PayloadType::elementMetaTypeId(), pb);
}

template<typename T>
void Item::setPayload(T *p)
{
    p->You_MUST_NOT_use_a_pointer_as_payload;
}

} // namespace Akonadi

Q_DECLARE_METATYPE(Akonadi::Item)
Q_DECLARE_METATYPE(Akonadi::Item::List)
Q_DECLARE_TYPEINFO(Akonadi::Item, Q_MOVABLE_TYPE);
