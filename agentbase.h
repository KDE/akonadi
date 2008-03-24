/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTBASE_H
#define AKONADI_AGENTBASE_H

#include "akonadi_export.h"

#include <KDE/KApplication>

#include <QtDBus/QDBusContext>

class AgentAdaptor;

namespace Akonadi {

class AgentBasePrivate;
class ChangeRecorder;
class Collection;
class Item;
class Session;

/**
  This calls is a base class for all Akonadi agents.

  It provides:
  - lifetime management
  - change monitoring and recording
  - configuration interface
  - problem reporting
*/
class AKONADI_EXPORT AgentBase : public QObject, protected QDBusContext
{
  Q_OBJECT

  public:
    /**
     * Use this method in the main function of your agent
     * application to initialize your agent subclass.
     * This method also takes care of creating a KApplication
     * object and parsing command line arguments.
     *
     * \code
     *
     *   class MyResource : public ResourceBase
     *   {
     *     ...
     *   };
     *
     *   int main( int argc, char **argv )
     *   {
     *     return AgentBase::init<MyAgent>( argc, argv );
     *   }
     *
     * \endcode
     */
    template <typename T>
    static int init( int argc, char **argv )
    {
      const QString id = parseArguments( argc, argv );
      KApplication app;
      T* r = new T( id );
      return init( r );
    }

    /**
     * This method is called whenever the resource shall show its configuration dialog
     * to the user. It will be automatically called when the resource is started for
     * the first time.
     * @param windowId The parent window id.
     */
    virtual void configure( WId windowId );

#ifdef Q_OS_WIN
    /**
     * Overload of @ref configure needed because WId cannot be automatically casted
     * to qlonglong on Windows.
     */
    //FIXME_API:
    void configure( qlonglong windowId ) { configure( reinterpret_cast<WId>( windowId ) ); }
#endif

    /**
     * Returns the instance identifier of this agent.
     */
    QString identifier() const;

    /**
     * This method is called when the agent is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup();

    /**
     * This method is called from the crash handler, don't call
     * it manually.
     */
    //FIXME_API: remove crashhandler
    void crashHandler( int signal );

  protected:
    /**
     * Creates a base agent.
     *
     * @param id The instance id of the agent.
     */
    AgentBase( const QString & id );

    /**
     * Destroys the base agent.
     */
    ~AgentBase();

    /**
     * This method shall be used to report warnings.
     */
    //FIXME_API: make them signals
    void warning( const QString& message );

    /**
     * This method shall be used to report errors.
     */
    //FIXME_API: make them signals
    void error( const QString& message );

    /**
     * This method is called whenever the application is about to
     * quit.
     *
     * Reimplement this method to do session cleanup (e.g. disconnecting
     * from groupware server).
     */
    virtual void aboutToQuit();

    /**
     * Returns a session for communicating with the storage backend. It should
     * be used for all jobs.
     */
    //FIXME_API: remove it and create default session in AgentBase ctor
    Session* session() const;

    /**
      Returns the Akonadi::ChangeRecorder object used for monitoring.
      Use this to configure which parts you want to monitor.
    */
    ChangeRecorder* changeRecorder() const;

    /**
      Marks the current change as processes and replays the next change if change
      recording is enabled (noop otherwise). This method is called
      from the default implementation of the change notification slots. While not
      required when not using change recording, it is nevertheless recommended to
      to call this method when done with processing a change notification.
    */
    //FIXME_API: move implementation to private class and make it virtual
    virtual void changeProcessed();

    //FIXME_API: make setOnline virtual + template method

  Q_SIGNALS:

  protected Q_SLOTS:
    //FIXME_API: move these 6 slots to new class EntityObserver.
    //FIXME_API: inherit from this observer and connect dbus signals to them.

    /**
      Reimplement to handle adding of new items.
      @param item The newly added item.
      @param collection The collection @p item got added to.
    */
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /**
      Reimplement to handle changes to existing items.
      @param item The changed item.
      @param partIdentifiers The identifiers of the item parts that has been changed.
    */
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers );

    /**
      Reimplement to handle deletion of items.
      @param item The deleted item.
    */
    virtual void itemRemoved( const Akonadi::Item &item );

    /**
      Reimplement to handle adding of new collections.
      @param collection The newly added collection.
      @param parent The parent collection.
    */
    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );

    /**
      Reimplement to handle changes to existing collections.
      @param collection The changed collection.
    */
    virtual void collectionChanged( const Akonadi::Collection &collection );

    /**
      Reimplement to handle deletion of collections.
      @param id The id of the deleted collection.
      @param remoteId The remote id of the deleted collection.
    */
    virtual void collectionRemoved( const Akonadi::Collection &collection );

  protected:
    //@cond PRIVATE
    AgentBasePrivate *d_ptr;
    explicit AgentBase( AgentBasePrivate* d, const QString &id );
    //@endcond

  private Q_SLOTS: //FIXME_API: accessed privately by adapter
    /**
     * This method is called to quit the agent.
     *
     * Before the application is terminated @see aboutToQuit() is called,
     * which can be reimplemented to do some session cleanup.
     */
    void quit();

  private:
    static QString parseArguments( int, char** );
    static int init( AgentBase *r );

    // dbus resource interface
    friend class ::AgentAdaptor;

    Q_DECLARE_PRIVATE( AgentBase )
    Q_PRIVATE_SLOT( d_func(), void delayedInit() )
};

}
/*
class DefaultObserver
{
  virtual itemAdded
  ..
}

class AgentBase
{
  AgentBase
  {
  }

  registerObserver( Observer *ob )
  {
    connect(mon, ob);
  }
  registerObserver2( Observer2 *ob )
  {
    connect(mon, ob);
  }
}

ExampleAgent : public AgentBase, public DefaultObserver

ExampleResource : public ResourceBase, public DefaultObserver
*/
#endif
