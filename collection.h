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

#include <libakonadi/job.h>

namespace PIM {

/**
  This class presents a collection of PIM objects, such as a folder on a mail or
  groupware server.

  Collections are hierarchical, i.e. they may have a parent collection.
 */
class Collection
{
  public:
    /**
      Collection types.
    */
    enum Type {
      Folder, /**< 'Real' folder on eg. an IMAP server. */
      Virtual, /**< Virtual collection (aka search folder). */
      Resource, /**< Resource or account. */
      Category, /**< Category. */
      Unknown /**< Unknown collection type. */
    };

    /**
      Create a new collection.

      @param ref The data reference of this collection.
    */
    Collection( const DataReference &ref );

    /**
      Destroys this collection.
    */
    virtual ~Collection();

    /**
      Returns the DataReference of this object.
    */
    DataReference reference() const;

    /**
      Returns the name of this collection.
    */
    QString name() const;

    /**
      Sets the name of this collection.
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
    QStringList contentTypes() const;

    /**
      Sets the list of possible content mimetypes.
    */
    void setContentTypes( const QStringList &types );

    /**
      Returns a reference to the parent collection, might be an empty reference if this
      collection is a top-level collection.
    */
    DataReference parent() const;

    /**
      Sets the parent collections.
    */
    void setParent( const DataReference &parent );

    /**
      Returns a query for the Akonadi backend to list the folder content.
    */
    QString query() const;

    /**
      Set the content query.
    */
    void setQuery( const QString &query );

    /**
      Copy the content of the given collection into this one.
      Used in PIM::CollectionModel to handle updates without changing pointers.

      Note: Subclasses should overwrite this method, copy their data and call
      the implementation of the super class.
    */
    virtual void copy( Collection *col );

  private:
    class CollectionPrivate;
    CollectionPrivate *d;
};

}

#endif
