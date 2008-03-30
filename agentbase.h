/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

class ControlAdaptor;
class StatusAdaptor;

namespace Akonadi {

class AgentBasePrivate;
class ChangeRecorder;
class Collection;
class Item;
class Session;

/**
 * This calls is a base class for all Akonadi agents.
 *
 * It provides:
 * - lifetime management
 * - change monitoring and recording
 * - configuration interface
 * - problem reporting
 */
class AKONADI_EXPORT AgentBase : public QObject, protected QDBusContext
{
  Q_OBJECT

  public:
    /**
     * The Observer provides an interface to react on monitored or replayed changes.
     *
     * Since the this base class does only tell the change recorder that the change
     * has been processed, an AgentBase subclass which wants to actually process
     * the change needs to subclass Observer and reimplement the methods it is
     * interested in.
     *
     * Such an agent specific Observer implementation can either be done
     * stand-alone, i.e. as a separate object, or by inheriting both AgentBase
     * and AgentBase::Observer.
     *
     * The observer implementation then has registered with the agent, so it
     * can forward the incoming changes to the observer.
     *
     * @note In the multiple inheritance approach the init() method automatically
     *       registers itself as the observer.
     *
     * Example for stand-alone observer:
     * @code
     * class ExampleAgent : public AgentBase
     * {
     *   public:
     *     ExampleAgent( const QString &id );
     *
     *     ~ExampleAgent();
     *
     *   private:
     *     AgentBase::Observer *mObserver;
     * };
     *
     * class ExampleObserver : public AgentBase::Observer
     * {
     *   protected:
     *     void itemChanged( const Item &item );
     * };
     *
     * ExampleAgent::ExampleAgent( const QString &id )
     *   : AgentBase( id ), mObserver( 0 )
     * {
     *   mObserver = new ExampleObserver();
     *   registerObserver( mObserver );
     * }
     *
     * ExampleAgent::~ExampleAgent()
     * {
     *   delete mObserver;
     * }
     *
     * void ExampleObserver::itemChanged( const Item &item )
     * {
     *   // do something with item
     *   kDebug() << "Item id=" << item.id();
     *
     *   // let base implementation tell the change recorder that we
     *   // have processed the change
     *   AgentBase::Observer::itemChanged( item );
     * }
     * @endcode
     *
     * Example for observer through multiple inheritance:
     * @code
     * class ExampleAgent : public AgentBase, public AgentBase::Observer
     * {
     *   public:
     *     ExampleAgent( const QString &id );
     *
     *   protected:
     *     void itemChanged( const Item &item );
     * };
     *
     * ExampleAgent::ExampleAgent( const QString &id )
     *   : AgentBase( id )
     * {
     *   // no need to create or register observer since
     *   // we are the observer and registration happens automatically
     *   // in init()
     * }
     *
     * void ExampleAgent::itemChanged( const Item &item )
     * {
     *   // do something with item
     *   kDebug() << "Item id=" << item.id();
     *
     *   // let base implementation tell the change recorder that we
     *   // have processed the change
     *   AgentBase::Observer::itemChanged( item );
     * }
     * @endcode
     */
    class AKONADI_EXPORT Observer  // krazy:exclude=dpointer
    {
      public:
        /**
         * Creates an observer instance
         */
        Observer();

        /**
         * Destroys the observer instance
         */
        virtual ~Observer();
        /**
         * Reimplement to handle adding of new items.
         * @param item The newly added item.
         * @param collection The collection @p item got added to.
         */
        virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

        /**
         * Reimplement to handle changes to existing items.
         * @param item The changed item.
         * @param partIdentifiers The identifiers of the item parts that has been changed.
         */
        virtual void itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers );

        /**
         * Reimplement to handle deletion of items.
         * @param item The deleted item.
         */
        virtual void itemRemoved( const Akonadi::Item &item );

        /**
         * Reimplement to handle adding of new collections.
         * @param collection The newly added collection.
         * @param parent The parent collection.
          */
        virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );

        /**
         * Reimplement to handle changes to existing collections.
         * @param collection The changed collection.
         */
        virtual void collectionChanged( const Akonadi::Collection &collection );

        /**
         * Reimplement to handle deletion of collections.
         * @param id The id of the deleted collection.
         * @param remoteId The remote id of the deleted collection.
         */
        virtual void collectionRemoved( const Akonadi::Collection &collection );
    };

    /**
     * This enum describes the different states the
     * agent can be in.
     */
    enum Status
    {
      Idle = 0,
      Running,
      Broken
    };

    /**
     * Use this method in the main function of your agent
     * application to initialize your agent subclass.
     * This method also takes care of creating a KApplication
     * object and parsing command line arguments.
     *
     * @note In case the given class is also derived from AgentBase::Observer
     *       it gets registered as its own observer (see AgentBase::Observer), e.g.
     *       <tt>agentInstance->registerObserver( agentInstance );</tt>
     *
     * @code
     *
     *   class MyAgent : public AgentBase
     *   {
     *     ...
     *   };
     *
     *   AKONADI_AGENT_MAIN( MyAgent )
     *
     * @endcode
     */
    template <typename T>
    static int init( int argc, char **argv )
    {
      const QString id = parseArguments( argc, argv );
      KApplication app;
      T* r = new T( id );

      // check if T also inherits AgentBase::Observer and
      // if it does, automatically register it on itself
      Observer *observer = dynamic_cast<Observer*>( r );
      if ( observer != 0 )
        r->registerObserver( observer );
      return init( r );
    }

    /**
     * This method returns the current status code of the resource.
     *
     * The following return values are possible:
     *
     *  0 - Idle
     *  1 - Running
     *  2 - Broken
     */
    virtual int status() const;

    /**
     * This method returns an i18n'ed description of the current status code.
     */
    virtual QString statusMessage() const;

    /**
     * This method returns the current progress of the resource in percentage.
     */
    virtual int progress() const;

    /**
     * This method returns an i18n'ed description of the current progress.
     */
    virtual QString progressMessage() const;

    /**
     * This method is called whenever the resource shall show its configuration dialog
     * to the user. It will be automatically called when the resource is started for
     * the first time.
     * @param windowId The parent window id.
     */
    virtual void configure( WId windowId );

    /**
     * This method returns the winId which should be used for dialogs.
     */
    WId winIdForDialogs() const;

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
     * Registers the given observer for reacting on monitored or recorded changes.
     *
     * @param observer The change handler to register. No ownership transfer, i.e.
     *                 the caller stays owner of the pointer and can reset
     *                 the registration by calling this method with @c 0
     */
    void registerObserver( Observer *observer );

  Q_SIGNALS:
    /**
     */
    void status( int status, const QString &message = QString() );

    /**
     */
    void percent( int progress );

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
     * Returns the Akonadi::ChangeRecorder object used for monitoring.
     * Use this to configure which parts you want to monitor.
     */
    ChangeRecorder* changeRecorder() const;

    /**
     * Marks the current change as processes and replays the next change if change
     * recording is enabled (noop otherwise). This method is called
     * from the default implementation of the change notification slots. While not
     * required when not using change recording, it is nevertheless recommended to
     * to call this method when done with processing a change notification.
     */
    void changeProcessed();

    bool isOnline() const;

    void setOnline( bool state );

  protected:
    //@cond PRIVATE
    AgentBasePrivate *d_ptr;
    explicit AgentBase( AgentBasePrivate* d, const QString &id );
    //@endcond

    /**
     * This method is called whenever the @p online status has changed.
     */
    virtual void doSetOnline( bool online );

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
    friend class ::StatusAdaptor;
    friend class ::ControlAdaptor;

    Q_DECLARE_PRIVATE( AgentBase )
    Q_PRIVATE_SLOT( d_func(), void delayedInit() )
    Q_PRIVATE_SLOT( d_func(), void slotStatus( int, const QString& ) )
    Q_PRIVATE_SLOT( d_func(), void slotPercent( int ) )
};

}

#ifndef AKONADI_AGENT_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi agents.
 */
#define AKONADI_AGENT_MAIN( agentClass )                       \
  int main( int argc, char **argv )                            \
  {                                                            \
    return Akonadi::AgentBase::init<agentClass>( argc, argv ); \
  }
#endif

#endif
