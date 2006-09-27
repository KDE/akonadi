/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_COLLECTION_H
#define PIM_COLLECTION_H

#include <libakonadi/collectionattribute.h>
#include <libakonadi/job.h>
#include <kdepim_export.h>

namespace PIM {

/**
  This class represents a collection of PIM objects, such as a folder on a mail- or
  groupware-server.

  Collections are hierarchical, i.e. they may have a parent collection.
 */
class AKONADI_EXPORT Collection
{
  public:
    /**
      Collection types.
    */
    enum Type {
      Folder, /**< 'Real' folder on eg. an IMAP server. */
      Virtual, /**< Virtual collection (aka search folder). */
      Resource, /**< Resource or account. */
      VirtualParent, /**< The parent collection of all virtual collections. */
      Unknown /**< Unknown collection type. */
    };

    typedef QList<Collection*> List;

    /**
      Create a new collection.

      @param path The unique IMAP path of this collection.
    */
    Collection( const QString &path );

    /**
      Destroys this collection.
    */
    virtual ~Collection();

    /**
      Returns the unique IMAP path of this collection.
    */
    QString path() const;

    /**
      Returns the name of this collection usable for display.
    */
    QString name() const;

    /**
      Sets the collection name. Note that this does not change path()!
      This is used during renaming to fake immediate changes.
      @param name The new collection name;
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
      Returns a list of possible content mimetypes,
      e.g. message/rfc822, x-akonadi/collection for a mail folder that
      supports sub-folders.
    */
    QList<QByteArray> contentTypes() const;

    /**
      Sets the list of possible content mimetypes.
    */
    void setContentTypes( const QList<QByteArray> &types );

    /**
      Returns the IMAP path to the parent collection.
    */
    QString parent() const;

    /**
      Sets the path of the parent collection.
    */
    void setParent( const QString &parent );

    /**
      Adds a collection attribute. An already existing attribute of the same
      type is deleted.
      @param attr The new attribute. The collection takes the ownership of this
      object.
    */
    void addAttribute( CollectionAttribute *attr );

    /**
      Returns true if the collection has the speciefied attribute.
      @param type The attribute type.
    */
    bool hasAttribute( const QByteArray &type ) const;

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
      Returns the collection path delimiter.
    */
    static QString delimiter();

    /**
      Returns the root path.
    */
    static QString root();

    /**
      Returns the Akonadi IMAP collection prefix.
    */
    static QString prefix();

    /**
      Returns the path of the top-level search folder.
    */
    static QString searchFolder();

    /**
      Returns the mimetype used for collections.
    */
    static QByteArray collectionMimeType();

  private:
    class Private;
    Private *d;
};

}

#endif
