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

#include <Akonadi/Collection>

class KJob;

namespace Akonadi {

class LocalFoldersPrivate;

/**
  @short Interface to local folders such as inbox, outbox etc.

  TODO: update this and method API docs...

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
      than one folder of each type.
    */
    enum Type
    {
      Root,            ///< the root folder containing the local folders
                       // TODO ^ do we even need this?
      Inbox,           ///< inbox
      Outbox,          ///< outbox
      SentMail,        ///< sent-mail
      Trash,           ///< trash
      Drafts,          ///< drafts
      Templates,       ///< templates
      LastDefaultType  ///< @internal marker
    };

    /**
      Returns the LocalFolders instance.
    */
    static LocalFolders *self();

    bool hasFolder( int type, const QString &resourceId ) const;

    Akonadi::Collection folder( int type, const QString &resourceId ) const;

    /// needs attribute and resource
    bool registerFolder( const Akonadi::Collection &collection );

    QString defaultResourceId() const;

    bool hasDefaultFolder( int type ) const;

    Akonadi::Collection defaultFolder( int type ) const;

  Q_SIGNALS:
    void foldersChanged( const QString &resourceId );

    /**
    */
    void defaultFoldersChanged();

  private:
    friend class LocalFoldersRequestJob;
    friend class LocalFoldersRequestJobPrivate;
    friend class LocalFoldersPrivate;

#if 1 // TODO do this only if building tests:
    friend class LocalFoldersTesting;
#endif

    // singleton class; the only instance resides in sInstance->instance
    LocalFolders( LocalFoldersPrivate *dd );

    void forgetFoldersForResource( const QString &resourceId );
    void beginBatchRegister();
    void endBatchRegister();

    LocalFoldersPrivate *const d;

    Q_PRIVATE_SLOT( d, void collectionRemoved( Akonadi::Collection ) )
};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERS_H
