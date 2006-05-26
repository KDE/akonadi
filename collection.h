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
    Collection( const QByteArray &path );

    /**
      Copy constructor.

      @param other The collection to copy.
    */
    Collection( const Collection &other );

    /**
      Destroys this collection.
    */
    virtual ~Collection();

    /**
      Returns the unique IMAP path of this collection.
    */
    QByteArray path() const;

    /**
      Returns the name of this collection usable for display.
    */
    QString name() const;

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
      Returns the IMAP path to the parent collection.
    */
    QByteArray parent() const;

    /**
      Sets the path of the parent collection.
    */
    void setParent( const QByteArray &parent );

    /**
      Returns the collection path delimiter.
    */
    static QByteArray delimiter();

    /**
      Returns the root path.
    */
    static QByteArray root();

    /**
      Returns the Akonadi IMAP collection prefix.
    */
    static QByteArray prefix();

    /**
      Returns the path of the top-level search folder.
    */
    static QByteArray searchFolder();

  private:
    class Private;
    Private *d;
};

}

#endif
