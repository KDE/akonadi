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

#include "akonadi/collection.h"

class KJob;

namespace Akonadi {

class LocalFoldersPrivate;

/**
  @short Interface to local folders such as inbox, outbox etc.

  This class is the central interface to the local mail folders. These folders
  can either be in the default resource (stored in ~/.local/share/local-mail)
  or in any number of custom resources. LocalFolders of the following types
  are supported: inbox, outbox, sent-mail, trash, drafts, and templates.

  To check whether a LocalFolder is available, simply use the hasFolder() and
  hasDefaultFolder() methods. Available LocalFolders are accessible through
  the folder() and defaultFolder() methods.

  There are two ways to create LocalFolders that are not yet available:
  1) Use a LocalFoldersRequestJob. This will create the LocalFolders you
     request and automatically register them with LocalFolders, so that it
     now knows they are available.
  2) Mark any collection as a LocalFolder, by giving it a LocalFolderAttribute,
     and then register it manually with registerFolder(). After that it will
     be known as a LocalFolder of the given type in the given resource.

  This class monitors all LocalFolder collections known to it, and removes it
  from the known list if they are deleted. Note that this class does not
  automatically rebuild the folders that disappeared.

  The defaultFoldersChanged() and foldersChanged() signals are emitted when
  the LocalFolders for a resource change (i.e. some became available or some
  become unavailable).

  @code
  if( LocalFolders::self()->hasDefaultFolder( LocalFolders::Outbox ) ) {
    Collection col = LocalFolders::self()->defaultFolder( LocalFolders::Outbox );
    // ...
  } else {
    // ... use LocalFoldersRequestJob to request the folder...
  }
  @endcode

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT LocalFolders : public QObject
{
  Q_OBJECT

  public:
    /**
      Each LocalFolder has one of the types below.  Generally, there may not
      be two LocalFolders of the same type in the same resource.
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

    /**
      Returns whether the given resource had a LocalFolder of the given
      @p type.
      @param type The type of the LocalFolder.
      @param resourceId The ID of the resource containing the LocalFolder.
    */
    bool hasFolder( int type, const QString &resourceId ) const;

    /**
      Returns the LocalFolder of given @p type in the given resource, or
      an invalid Collection if such a folder is unknown.
      @param type The type of the LocalFolder.
      @param resourceId The ID of the resource containing the LocalFolder.
    */
    Akonadi::Collection folder( int type, const QString &resourceId ) const;

    /**
      Registers the given @p collection as a LocalFolder. The collection must
      have a valid LocalFolderAttribute determining its LocalFolder type, and
      it must be owned by a valid resource.
      Registering a new collection of a previously registered type forgets the
      old collection.
      @param collection The folder to register.
      @return Whether registration was successful.
    */
    bool registerFolder( const Akonadi::Collection &collection );

    /**
      Returns the ID of the default LocalFolders resource.
      This resource is normally stored in ~/.local/share/local-mail, and its
      ID is stored in a config file named localfoldersrc.
    */
    QString defaultResourceId() const;

    /**
      Returns whether the default resource had a LocalFolder of the given
      @p type.
      @param type The type of the LocalFolder.
    */
    bool hasDefaultFolder( int type ) const;

    /**
      Returns the LocalFolder of given @p type in the default resource, or
      an invalid Collection if such a folder is unknown.
      @param type The type of the LocalFolder.
    */
    Akonadi::Collection defaultFolder( int type ) const;

  Q_SIGNALS:
    /**
      Emitted when the LocalFolders for a resource are changed (for example,
      some become available, or some become unavailable).
      @param resourceId The ID of the resource for which the folders changed.
    */
    // TODO: Currently when a resource is deleted, all its collections are
    // deleted too, and foldersChanged() is emitted multiple times. This
    // doesn't seem to be a problem, but it would be nice if this signal was
    // delayed and emitted only once.
    void foldersChanged( const QString &resourceId );

    /**
      Emitted when the LocalFolders for the default resource are changed (for
      example, some become available, or some become unavailable).
      Note that when the default folders change, both defaultFoldersChanged()
      and foldersChanged( LocalFolders::self()->defaultResourceId() ) are
      emitted.
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

    /**
      Forgets all folders owned by the given resource.
      This method is used by LocalFoldersRequestJob.
    */
    void forgetFoldersForResource( const QString &resourceId );

    /**
      Avoids emitting the foldersChanged() signal until endBatchRegister()
      is called. This is used to avoid emitting repeated signals when multiple
      folders are registered in a row.
      This method is used by LocalFoldersRequestJob.
    */
    void beginBatchRegister();

    /**
      @see beginBatchRegister()
      This method is used by LocalFoldersRequestJob.
    */
    void endBatchRegister();

    LocalFoldersPrivate *const d;

    Q_PRIVATE_SLOT( d, void collectionRemoved( Akonadi::Collection ) )
};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERS_H
