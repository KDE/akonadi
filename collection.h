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

#include "akonadi_export.h"

#include <akonadi/entity.h>

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

class KUrl;

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
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchFinished( KJob* ) ) );
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
 *   foreach( const Collection &collection, collections ) {
 *     qDebug() << "Name:" << collection.name();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT Collection : public Entity
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
      CanChangeCollection = 0x8,        ///< Can change subcollections in this collection
      CanCreateCollection = 0x10,       ///< Can create new subcollections in this collection
      CanDeleteCollection = 0x20,       ///< Can delete subcollections in this collection
      CanLinkItem = 0x40,               ///< Can create links to existing items in this virtual collection @since 4.4
      CanUnlinkItem = 0x80,             ///< Can remove links to items in this virtual collection @since 4.4
      AllRights = (CanChangeItem | CanCreateItem | CanDeleteItem |
                   CanChangeCollection | CanCreateCollection | CanDeleteCollection) ///< Has all rights on this storage collection
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
    explicit Collection( Id id );

    /**
     * Destroys the collection.
     */
    ~Collection();

    /**
     * Creates a collection from an @p other collection.
     */
    Collection( const Collection &other );

    /**
     * Creates a collection from the given @p url.
     */
    static Collection fromUrl( const KUrl &url );

    /**
     * Returns the i18n'ed name of the collection.
     */
    QString name() const;

    /**
     * Sets the i18n'ed name of the collection.
     *
     * @param name The new collection name.
     */
    void setName( const QString &name );

    /**
     * Returns the rights the user has on the collection.
     */
    Rights rights() const;

    /**
     * Sets the @p rights the user has on the collection.
     */
    void setRights( Rights rights );

    /**
     * Returns a list of possible content mimetypes,
     * e.g. message/rfc822, x-akonadi/collection for a mail folder that
     * supports sub-folders.
    */
    QStringList contentMimeTypes() const;

    /**
     * Sets the list of possible content mime @p types.
     */
    void setContentMimeTypes( const QStringList &types );

    /**
     * Returns the identifier of the parent collection.
     * @deprecated Use parentCollection()
     */
    KDE_DEPRECATED Id parent() const;

    /**
     * Sets the identifier of the @p parent collection.
     * @deprecated Use setParentCollection()
     */
    KDE_DEPRECATED void setParent( Id parent );

    /**
     * Sets the parent @p collection.
     * @deprecated Use setParentCollection()
     */
    KDE_DEPRECATED void setParent( const Collection &collection );

    /**
     * Returns the parent remote identifier.
     * @note This usually returns nothing for collections retrieved from the backend.
     * @deprecated Use parentCollection()
     */
    KDE_DEPRECATED QString parentRemoteId() const;

    /**
     * Sets the parent's remote @p identifier.
     * @deprecated Use setParentCollection()
     */
    KDE_DEPRECATED void setParentRemoteId( const QString &identifier );

    /**
     * Returns the root collection.
     */
    static Collection root();

    /**
     * Returns the mimetype used for collections.
     */
    static QString mimeType();

    /**
     * Returns the identifier of the resource owning the collection.
     */
    QString resource() const;

    /**
     * Sets the @p identifier of the resource owning the collection.
     */
    void setResource( const QString &identifier );

    /**
     * Returns the cache policy of the collection.
     */
    CachePolicy cachePolicy() const;

    /**
     * Sets the cache @p policy of the collection.
     */
    void setCachePolicy( const CachePolicy &policy );

    /**
     * Returns the collection statistics of the collection.
     */
    CollectionStatistics statistics() const;

    /**
     * Sets the collection @p statistics for the collection.
     */
    void setStatistics( const CollectionStatistics &statistics );

    /**
     * Returns the collection url
     */
    KUrl url() const;

  private:
    AKONADI_DECLARE_PRIVATE( Collection )
    friend class CollectionFetchJob;
    friend class CollectionModifyJob;
};

}

AKONADI_EXPORT uint qHash( const Akonadi::Collection &collection );
/**
 * Allows to output a collection for debugging purposes.
 */
AKONADI_EXPORT QDebug operator<<( QDebug d, const Akonadi::Collection &collection );

Q_DECLARE_METATYPE(Akonadi::Collection)
Q_DECLARE_METATYPE(Akonadi::Collection::List)
Q_DECLARE_OPERATORS_FOR_FLAGS( Akonadi::Collection::Rights )

#endif
