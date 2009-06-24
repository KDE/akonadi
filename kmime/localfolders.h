/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_LOCALFOLDERS_H
#define AKONADI_LOCALFOLDERS_H

#include "akonadi-kmime_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>

#include <akonadi/collection.h>

class KJob;

namespace Akonadi {
  class Collection;
}


namespace Akonadi {


class LocalFoldersPrivate;


/**
  Creates and monitors the Outbox and Sent-Mail collections for the mail
  dispatcher agent.

  The first time it is used, or when you call fetch(), this class checks for
  the following:
  * a maildir resource with name 'Local Mail Folders'
  * 'outbox' and 'sent-mail' collections under that resource
  If they do not exist, this class creates them, and then emits foldersReady().

  Do not store the collections returned by outbox() and sentMail().  They can
  become invalid if e.g. the user deletes them and LocalFolders creates them
  again, so you should always use LocalFolders::outbox() instead of storing
  its return value.

*/
class AKONADI_KMIME_EXPORT LocalFolders : public QObject
{
  Q_OBJECT

  public:
    /**
      Each folder has one of the types below.  There is only one folder of
      each type, except for Custom, which multiple folders can use.
    */
    enum Type
    {
      Inbox,
      Outbox,
      SentMail,
      Trash,
      Drafts,
      Templates,
      LastDefaultType, //< internal
      Custom = 31 //< for custom folders created by the user
      //,User = 32 //< for user-defined types
    };

    /**
      Returns the LocalFolders instance.
      Does a fetch() when first called.
    */
    static LocalFolders *self();

    /**
      Returns whether the outbox and sent-mail collections have been
      fetched and are ready to be used via outbox() and sentMail().
    */
    bool isReady() const;

    /**
      Returns the inbox collection.
    */
    Akonadi::Collection inbox() const;

    /**
      Returns the outbox collection.
    */
    Akonadi::Collection outbox() const;

    /**
      Returns the sent-mail collection.
    */
    Akonadi::Collection sentMail() const;

    /**
      Returns the trash collection.
    */
    Akonadi::Collection trash() const;

    /**
      Returns the drafts collection.
    */
    Akonadi::Collection drafts() const;

    /**
      Returns the templates collection.
    */
    Akonadi::Collection templates() const;

#if 0
    /**
      Get a folder by its name.
      Returns an invalid collection if no such folder exists.
    */
    Akonadi::Collection folder( const QString &name ) const;
#endif

    /**
      Get a folder by its type.
      Returns an invalid collection if no such folder exists.
      For Custom folders, call folders() instead.
    */
    Akonadi::Collection folder( Type type ) const;

    /**
      Returns all folders of type @p type.
      If this is a default type (such as inbox, outbox etc.), then there is
      at most one folder of that type, so you may call folder() instead.
    */
    Akonadi::Collection::List folders( Type type ) const;

  public Q_SLOTS:
    /**
      Begins creating / fetching the resource and collections.
      Emits foldersReady() when done.
    */
    void fetch();

  Q_SIGNALS:
    /**
      Emitted when the outbox and sent-mail collections have been fetched and
      are ready to be used via outbox() and sentMail().
    */
    void foldersReady();

  private:
    friend class LocalFoldersPrivate;

    // singleton class; the only instance resides in sInstance->instance
    LocalFolders( LocalFoldersPrivate *dd );

    LocalFoldersPrivate *const d;

    Q_PRIVATE_SLOT( d, void prepare() )
    Q_PRIVATE_SLOT( d, void schedulePrepare() )
    Q_PRIVATE_SLOT( d, void resourceCreateResult( KJob * ) )
    Q_PRIVATE_SLOT( d, void resourceSyncResult( KJob * ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob * ) )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob * ) )

};


}


#endif
