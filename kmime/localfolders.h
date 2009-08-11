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

#include <akonadi/collection.h>

class KJob;

namespace Akonadi {

class LocalFoldersPrivate;

/**
  @short Interface to local folders such as inbox, outbox etc.

  This class provides access to the following default (and undeletable) local
  folders: inbox, outbox, sent-mail, trash, drafts, and templates; as well as
  the root local folder containing them.  The user may create custom
  subfolders in the root local folder, or in any of the default local folders.

  By default, these folders are stored in a maildir in
  $HOME/.local/share/mail.

  This class also monitors the local folders and the maildir resource hosting
  them, and re-creates them when necessary (for example, if the user
  accidentally removed the maildir resource).  For this reason, you must make
  sure the local folders are ready before calling any of the accessor
  functions.  It is also recommended that you always use this class to access
  the local folders, instead of, for example, storing the result of
  LocalFolders::self()->outbox() locally.

  @code
  connect( LocalFolders::self(), SIGNAL(foldersReady()),
      this, SLOT(riseAndShine()) );
  LocalFolders::self()->fetch();
  @endcode

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT LocalFolders : public QObject
{
  Q_OBJECT

  public:
    /**
      Each local folder has one of the types below.  There cannot be more
      than one folder of each type, except for Custom.
    */
    enum Type
    {
      Root,            ///< the root folder containing the local folders
      Inbox,           ///< inbox
      Outbox,          ///< outbox
      SentMail,        ///< sent-mail
      Trash,           ///< trash
      Drafts,          ///< drafts
      Templates,       ///< templates
      LastDefaultType, ///< @internal marker
      Custom = 15      ///< for custom folders created by the user
    };

    /**
      Returns the LocalFolders instance.
      Does a fetch() when first called.

      @see fetch
    */
    static LocalFolders *self();

    /**
      Returns whether the local folder collections have been fetched and are
      ready to be accessed.

      @see fetch
      @see foldersReady
    */
    bool isReady() const;

    /**
      Returns the root collection containing all local folders.
    */
    Akonadi::Collection root() const;

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

    /**
      Get a folder by its type.
      Returns an invalid collection if no such folder exists.
      This function only works for default (non-Custom) folder types.

      @see Type
    */
    Akonadi::Collection folder( int type ) const;

  public Q_SLOTS:
    /**
      Begins fetching the resource and collections, or creating them if
      necessary.  Emits foldersReady() when done.

      @code
      connect( LocalFolders::self(), SIGNAL(foldersReady()),
          this, SLOT(riseAndShine()) );
      LocalFolders::self()->fetch();
      @endcode
    */
    void fetch();

  Q_SIGNALS:
    /**
      Emitted when the local folder collections have been fetched and
      are ready to be accessed.

      @see isReady
    */
    void foldersReady();

  private:
    friend class LocalFoldersPrivate;

    // singleton class; the only instance resides in sInstance->instance
    LocalFolders( LocalFoldersPrivate *dd );

    LocalFoldersPrivate *const d;

    Q_PRIVATE_SLOT( d, void prepare() )
    Q_PRIVATE_SLOT( d, void buildResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionRemoved( Akonadi::Collection ) )

};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERS_H
