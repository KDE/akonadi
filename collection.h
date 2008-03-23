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
#include <akonadi/cachepolicy.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/entity.h>

#include <QtCore/QSharedDataPointer>

class KUrl;

class CollectionPrivate;

namespace Akonadi {

class CollectionPrivate;

/**
  This class represents a collection of PIM objects, such as a folder on a mail- or
  groupware-server.

  Collections are hierarchical, i.e., they may have a parent collection.

  This class is implicitly shared.

  @see \ref akonadi_concepts_collections "Akonadi Collection Concept"
 */
class AKONADI_EXPORT Collection : public Entity
{
  public:
    /**
      Collection types.
    */
    enum Type {
      Folder, /**< 'Real' folder on an IMAP server for example. */
      Virtual, /**< Virtual collection (aka search folder). */
      Structural, /**< Structural node to keep the collection tree consistent but cannot contain any content. */
      Resource, /**< Resource or account. */
      VirtualParent, /**< The parent collection of all virtual collections. */
      Unknown /**< Unknown collection type. */
    };

    /// A list of collections.
    typedef QList<Collection> List;

    /**
     Collection rights.
    */
    enum Right {
      NoRights = 0x0,                   ///< Has no rights on this collection
      CanChangeItem = 0x1,              ///< Can change items in this collection
      CanCreateItem = 0x2,              ///< Can create new items in this collection
      CanDeleteItem = 0x4,              ///< Can delete items in this collection
      CanChangeCollection = 0x8,        ///< Can change subcollections in this collection
      CanCreateCollection = 0x16,       ///< Can create new subcollections in this collection
      CanDeleteCollection = 0x32,       ///< Can delete subcollections in this collection
      AllRights = (CanChangeItem | CanCreateItem | CanDeleteItem |
                   CanChangeCollection | CanCreateCollection | CanDeleteCollection) ///< Has all rights on this collection
    };
    Q_DECLARE_FLAGS(Rights, Right)

    /**
      Creates an invalid collection.
    */
    Collection();

    /**
      Create a new collection.

      @param id The unique identifier of this collection.
    */
    explicit Collection( Id id );

    /**
      Destroys this collection.
    */
    ~Collection();

    /**
      Copy constructor.
    */
    Collection( const Collection &other );

    /**
      Creates a collection from the url
    */
    static Collection fromUrl( const KUrl &url );

    /**
      Returns the name of this collection usable for display.
    */
    QString name() const;

    /**
      Sets the collection name. Note that this does not change path()!
      This is used during renaming to fake immediate changes.
      @param name The new collection name
    */
    void setName( const QString &name );

    /**
      Returns the type of this collection (e.g. virtual folder, folder on an
      IMAP server, etc.).
    */
    Type type() const;

    /**
      Sets the type of this collection.
    */
    void setType( Type type );

    /**
      Returns the rights the user has on this collection.
     */
    Rights rights() const;

    /**
      Sets the @p rights the user has on this collection.
     */
    void setRights( Rights rights );

    /**
      Returns a list of possible content mimetypes,
      e.g. message/rfc822, x-akonadi/collection for a mail folder that
      supports sub-folders.
    */
    QStringList contentTypes() const;

    /**
      Sets the list of possible content mimetypes.
    */
    void setContentTypes( const QStringList &types );

    /**
      Returns the identifier of the parent collection.
    */
    Id parent() const;

    /**
      Sets the identifier of the parent collection.
    */
    void setParent( Id parent );

    /**
      Sets the parent collection.
    */
    void setParent( const Collection &collection );

    /**
      Returns the parent remote identifier.
      Note: This usually returns nothing for collections retrieved from the backend.
    */
    QString parentRemoteId() const;

    /**
      Sets the parent remote identifier.
      @param remoteParent The remote identifier of the parent.
    */
    void setParentRemoteId( const QString &remoteParent );

    /**
      Returns the collection path delimiter.
    */
    static QString delimiter();

    /**
      Returns the root collection.
    */
    static Collection root();

    /**
      Returns the mimetype used for collections.
    */
    static QString collectionMimeType();

    /**
      Returns the identifier of the resource owning this collection.
    */
    QString resource() const;

    /**
      Sets the identifier of the resource owning this collection.
      @param resource The resource identifier.
    */
    void setResource( const QString &resource );

    /**
      Returns the cache policy of this collection.
    */
    CachePolicy cachePolicy() const;

    /**
      Sets the cache policy of this collection.
      @param cachePolicy The new cache policy.
    */
    void setCachePolicy( const CachePolicy &cachePolicy );

    /**
      Returns the CollectionStatistics object.
    */
    CollectionStatistics statistics() const;

    /**
      Sets the CollectionStatistics object for this collection.
    */
    void setStatistics( const CollectionStatistics &statistics );

    /**
      Returns the collection url
    */
    KUrl url() const;

    /**
      Returns true if the url is valid for an item
    */
    static bool urlIsValid( const KUrl &url );

  private:
    AKONADI_DECLARE_PRIVATE( Collection )
    friend class CollectionModifyJob;
};

}

AKONADI_EXPORT uint qHash( const Akonadi::Collection &collection );

Q_DECLARE_METATYPE(Akonadi::Collection)
Q_DECLARE_METATYPE(Akonadi::Collection::List)

#endif
