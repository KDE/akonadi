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

#include "libakonadi_export.h"
#include <libakonadi/collectionattribute.h>
#include <libakonadi/collectionstatus.h>
#include <libakonadi/cachepolicy.h>

#include <QtCore/QSharedDataPointer>

class KUrl;

namespace Akonadi {

/**
  This class represents a collection of PIM objects, such as a folder on a mail- or
  groupware-server.

  Collections are hierarchical, i.e., they may have a parent collection.

  This class is implicitly shared.

  @see \ref akonadi_concepts_collections "Akonadi Collection Concept"
 */
class AKONADI_EXPORT Collection
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
    explicit Collection( int id );

    /**
      Copy constructor.
    */
    Collection( const Collection &other );

    /**
      Destroys this collection.
    */
    ~Collection();

    /**
      Creates a collection from the url
    */
    static Collection fromUrl( const KUrl &url );

    /**
      Returns the unique identifier of this collection.
    */
    int id() const;

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
    int parent() const;

    /**
      Sets the identifier of the parent collection.
    */
    void setParent( int parent );

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
      Adds a collection attribute. An already existing attribute of the same
      type is deleted.
      @param attr The new attribute. The collection takes the ownership of this
      object.
    */
    void addAttribute( CollectionAttribute *attr );

    /**
      Returns true if the collection has the specified attribute.
      @param type The attribute type.
    */
    bool hasAttribute( const QByteArray &type ) const;

    /**
      Returns all attributes.
    */
    QList<CollectionAttribute*> attributes() const;

    /**
      Returns the attribute of the given type if available, 0 otherwise.
      @param type The attribute type.
    */
    CollectionAttribute* attribute( const QByteArray &type ) const;

    /**
      Returns the attribute of the requested type or 0 if not available.
      @param create Creates the attribute if it doesn't exist.
    */
    template <typename T> inline T* attribute( bool create = false )
    {
      T dummy;
      if ( hasAttribute( dummy.type() ) )
        return static_cast<T*>( attribute( dummy.type() ) );
      if ( !create )
        return 0;
      T* attr = new T();
      addAttribute( attr );
      return attr;
    }

    /**
      Returns the attribute of the requested type or 0 if not available.
    */
    template <typename T> inline T* attribute() const
    {
      T dummy;
      if ( hasAttribute( dummy.type() ) )
        return static_cast<T*>( attribute( dummy.type() ) );
      return 0;
    }

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
      Returns true if this is a valid collection.
    */
    bool isValid() const;

    /**
      Compares two collections.
    */
    bool operator==( const Collection &other ) const;

    /**
      Compares two collections.
    */
    bool operator!=( const Collection &other ) const;

    /**
      Returns the remote identifier of this collection.
    */
    QString remoteId() const;

    /**
      Sets the remote identifier of this collection.
    */
    void setRemoteId( const QString &remoteId );

    /**
      Assignment operator.
    */
    Collection& operator=( const Collection &other );

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
      Adds raw attribute data. If an attribute with the same name already exists
      it is overwritten.
      @param type Attribute type.
      @param value Attribute value.
    */
    void addRawAttribute( const QByteArray &type, const QByteArray &value );

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
      Returns the CollectionStatus object.
    */
    CollectionStatus status() const;

    /**
      Sets the CollectionStatus object for this collection.
    */
    void setStatus( const CollectionStatus &status );

    /**
      Returns the collection url
    */
    KUrl url() const;

    /**
      Returns true if the url is valid for an item
    */
    static bool urlIsValid( const KUrl &url );

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

AKONADI_EXPORT uint qHash( const Akonadi::Collection &collection );

Q_DECLARE_METATYPE(Akonadi::Collection)
Q_DECLARE_METATYPE(Akonadi::Collection::List)

#endif
