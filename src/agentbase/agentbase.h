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

#include "akonadiagentbase_export.h"
#include "item.h"

#include <KApplication>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusContext>

class Akonadi__ControlAdaptor;
class Akonadi__StatusAdaptor;

namespace Akonadi {

class AgentBasePrivate;
class ChangeRecorder;
class Collection;
class Item;

/**
 * @short The base class for all Akonadi agents and resources.
 *
 * This class is a base class for all Akonadi agents, which covers the real
 * agent processes and all resources.
 *
 * It provides:
 * - lifetime management
 * - change monitoring and recording
 * - configuration interface
 * - problem reporting
 *
 * Akonadi Server supports several ways to launch agents and resources:
 * - As a separate application (@see AKONADI_AGENT_MAIN)
 * - As a thread in the AgentServer
 * - As a separate process, using the akonadi_agent_launcher
 *
 * The idea is this, the agent or resource is written as a plugin instead of an
 * executable (@see AgentFactory). In the AgentServer case, the AgentServer
 * looks up the plugin and launches the agent in a separate thread. In the
 * launcher case, a new akonadi_agent_launcher process is started for each
 * agent or resource instance.
 *
 * When making an Agent or Resource suitable for running in the AgentServer some
 * extra caution is needed. Because multiple instances of several kinds of agents
 * run in the same process, one cannot blindly use global objects like KGlobal.
 * For this reasons several methods where added to avoid problems in this context,
 * most notably AgentBase::config() and AgentBase::componentData(). Additionally,
 * one cannot use QDBusConnection::sessionBus() with dbus < 1.4, because of a
 * multithreading bug in libdbus. Instead one should use
 * DBusConnectionPool::threadConnection() which works around this problem.
 *
 * @author Till Adam <adam@kde.org>, Volker Krause <vkrause@kde.org>
 */
class AKONADIAGENTBASE_EXPORT AgentBase : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    /**
     * @short The interface for reacting on monitored or replayed changes.
     *
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
     * @note Do not call the base implementation of reimplemented virtual methods!
     *       The default implementation disconnected themselves from the Akonadi::ChangeRecorder
     *       to enable internal optimizations for unused notifications.
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
         : AgentBase( id )
         , mObserver( 0 )
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
     *   qDebug() << "Item id=" << item.id();
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
         : AgentBase( id )
     * {
     *   // no need to create or register observer since
     *   // we are the observer and registration happens automatically
     *   // in init()
     * }
     *
     * void ExampleAgent::itemChanged( const Item &item )
     * {
     *   // do something with item
     *   qDebug() << "Item id=" << item.id();
     *
     *   // let base implementation tell the change recorder that we
     *   // have processed the change
     *   AgentBase::Observer::itemChanged( item );
     * }
     * @endcode
     *
     * @author Kevin Krammer <kevin.krammer@gmx.at>
     *
     * @deprecated Use ObserverV2 instead
     */
    class AKONADIAGENTBASE_EXPORT Observer  // krazy:exclude=dpointer
    {
    public:
        /**
         * Creates an observer instance.
         */
        Observer();

        /**
         * Destroys the observer instance.
         */
        virtual ~Observer();

        /**
         * Reimplement to handle adding of new items.
         * @param item The newly added item.
         * @param collection The collection @p item got added to.
         */
        virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);

        /**
         * Reimplement to handle changes to existing items.
         * @param item The changed item.
         * @param partIdentifiers The identifiers of the item parts that has been changed.
         */
        virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers);

        /**
         * Reimplement to handle deletion of items.
         * @param item The deleted item.
         */
        virtual void itemRemoved(const Akonadi::Item &item);

        /**
         * Reimplement to handle adding of new collections.
         * @param collection The newly added collection.
         * @param parent The parent collection.
          */
        virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);

        /**
         * Reimplement to handle changes to existing collections.
         * @param collection The changed collection.
         */
        virtual void collectionChanged(const Akonadi::Collection &collection);

        /**
         * Reimplement to handle deletion of collections.
         * @param collection The deleted collection.
         */
        virtual void collectionRemoved(const Akonadi::Collection &collection);
    };

    /**
     * BC extension of Observer with support for monitoring item and collection moves.
     * Use this one instead of Observer.
     *
     * @since 4.4
     */
    class AKONADIAGENTBASE_EXPORT ObserverV2 : public Observer  // krazy:exclude=dpointer
    {
    public:
        using Observer::collectionChanged;

        /**
         * Reimplement to handle item moves.
         * When using this class in combination with Akonadi::ResourceBase, inter-resource
         * moves are handled internally already and the corresponding  add or delete method
         * is called instead.
         *
         * @param item The moved item.
         * @param collectionSource The collection the item has been moved from.
         * @param collectionDestination The collection the item has been moved to.
         */
        virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                               const Akonadi::Collection &collectionDestination);

        /**
         * Reimplement to handle item linking.
         * This is only relevant for virtual resources.
         * @param item The linked item.
         * @param collection The collection the item is linked to.
         */
        virtual void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

        /**
         * Reimplement to handle item unlinking.
         * This is only relevant for virtual resources.
         * @param item The unlinked item.
         * @param collection The collection the item is unlinked from.
         */
        virtual void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

        /**
         * Reimplement to handle collection moves.
         * When using this class in combination with Akonadi::ResourceBase, inter-resource
         * moves are handled internally already and the corresponding  add or delete method
         * is called instead.
         *
         * @param collection The moved collection.
         * @param collectionSource The previous parent collection.
         * @param collectionDestination The new parent collection.
         */
        virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                     const Akonadi::Collection &collectionDestination);

        /**
         * Reimplement to handle changes to existing collections.
         * @param collection The changed collection.
         * @param changedAttributes The identifiers of the collection parts/attributes that has been changed.
         */
        virtual void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes);
    };

    /**
     * BC extension of ObserverV2 with support for batch operations
     *
     * @warning When using ObserverV3, you will never get single-item notifications
     * from AgentBase::Observer, even when you don't reimplement corresponding batch
     * method from ObserverV3. For instance, when you don't reimplement itemsRemoved()
     * here, you will not get any notifications about item removal whatsoever!
     *
     * @since 4.11
     */
    class AKONADIAGENTBASE_EXPORT ObserverV3 : public ObserverV2 // krazy:exclude=dpointer
    {
    public:
        /**
         * Reimplement to handle changes in flags of existing items
         *
         * @warning When using ObserverV3, you will never get notifications about
         * flag changes via Observer::itemChanged(), even when you don't reimplement
         * itemsFlagsChanged()!
         *
         * @param item The changed item.
         * @param addedFlags Flags that have been added to the item
         * @param removedFlags Flags that have been removed from the item
         */
        virtual void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags);

        /**
         * Reimplement to handle batch notification about items deletion.
         *
         * @param items List of deleted items
         */
        virtual void itemsRemoved(const Akonadi::Item::List &items);

        /**
         * Reimplement to handle batch notification about items move
         *
         * @param items List of moved items
         * @param sourceCollection Collection from where the items were moved
         * @param destinationCollection Collection to which the items were moved
         */
        virtual void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection,
                                const Akonadi::Collection &destinationCollection);

        /**
         * Reimplement to handle batch notifications about items linking.
         *
         * @param items Linked items
         * @param collection Collection to which the items have been linked
         */
        virtual void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);

        /**
         * Reimplement to handle batch notifications about items unlinking.
         *
         * @param items Unlinked items
         * @param collection Collection from which the items have been unlinked
         */
        virtual void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);
    };

    /**
     * Observer that adds support for item tagging
     *
     * @warning ObserverV4 subclasses ObserverV3 which changes behavior of some of the
     * virtual methods from Observer and ObserverV2. Please make sure you read
     * documentation of ObserverV3 and adapt your agent accordingly.
     *
     * @since 4.13
     */
    class AKONADIAGENTBASE_EXPORT ObserverV4 : public ObserverV3 // krazy:exclude=dpointer
    {
    public:
        /**
         * Reimplement to handle tags additions
         *
         * @param tag Newly added tag
         */
        virtual void tagAdded(const Akonadi::Tag &tag);

        /**
         * Reimplement to handle tags changes
         *
         * @param tag Tag that has been changed
         */
        virtual void tagChanged(const Akonadi::Tag &tag);

        /**
         * Reimplement to handle tags removal.
         *
         * @note All items that were tagged by @p tag will get a separate notification
         * about untagging via itemsTagsChanged(). It is guaranteed that the itemsTagsChanged()
         * notification will be delivered before this one.
         *
         * @param tag Tag that has been removed.
         */
        virtual void tagRemoved(const Akonadi::Tag &tag);

        /**
         * Reimplement to handle items tagging
         *
         * @param items Items that were tagged or untagged
         * @param addedTags Set of tags that were added to all @p items
         * @param removedTags Set of tags that were removed from all @p items
         */
        virtual void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags);
    };

    /**
     * This enum describes the different states the
     * agent can be in.
     */
    enum Status {
        Idle = 0, ///< The agent does currently nothing.
        Running,  ///< The agent is working on something.
        Broken,    ///< The agent encountered an error state.
        NotConfigured ///< The agent is lacking required configuration
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
     *
     * @param argc number of arguments
     * @param argv arguments for the function
     */
    template <typename T>
    static int init(int argc, char **argv)
    {
        const QString id = parseArguments(argc, argv);
        KApplication app;
        T *r = new T(id);

        // check if T also inherits AgentBase::Observer and
        // if it does, automatically register it on itself
        Observer *observer = dynamic_cast<Observer *>(r);
        if (observer != 0) {
            r->registerObserver(observer);
        }
        return init(r);
    }

    /**
     * This method returns the current status code of the agent.
     *
     * The following return values are possible:
     *
     *  - 0 - Idle
     *  - 1 - Running
     *  - 2 - Broken
     *  - 3 - NotConfigured
     */
    virtual int status() const;

    /**
     * This method returns an i18n'ed description of the current status code.
     */
    virtual QString statusMessage() const;

    /**
     * This method returns the current progress of the agent in percentage.
     */
    virtual int progress() const;

    /**
     * This method returns an i18n'ed description of the current progress.
     */
    virtual QString progressMessage() const;

public Q_SLOTS:
    /**
     * This method is called whenever the agent shall show its configuration dialog
     * to the user. It will be automatically called when the agent is started for
     * the first time.
     *
     * @param windowId The parent window id.
     *
     * @note If the method is reimplemented it has to emit the configurationDialogAccepted()
     *       or configurationDialogRejected() signals depending on the users choice.
     */
    virtual void configure(WId windowId);

public:
    /**
     * This method returns the windows id, which should be used for dialogs.
     */
    WId winIdForDialogs() const;

#ifdef Q_OS_WIN
    /**
     * Overload of @ref configure needed because WId cannot be automatically casted
     * to qlonglong on Windows.
     */
    void configure(qlonglong windowId);
#endif

    /**
     * Returns the instance identifier of this agent.
     */
    QString identifier() const;

    /**
     * This method is called when the agent is removed from
     * the system, so it can do some cleanup stuff.
     *
     * @note If you reimplement this in a subclass make sure
     *       to call this base implementation at the end.
     */
    virtual void cleanup();

    /**
     * Registers the given observer for reacting on monitored or recorded changes.
     *
     * @param observer The change handler to register. No ownership transfer, i.e.
     *                 the caller stays owner of the pointer and can reset
     *                 the registration by calling this method with @c 0
     */
    void registerObserver(Observer *observer);

    /**
     * This method is used to set the name of the agent.
     *
     * @since 4.3
     * @param name name of the agent
     */
    //FIXME_API: make sure location is renamed to this by agentbase
    void setAgentName(const QString &name);

    /**
     * Returns the name of the agent.
     *
     * @since 4.3
     */
    QString agentName() const;

    /**
     * Returns the component data object for this agent instance.
     * In case of stand-alone agents this is identical to KGlobal::mainComponent().
     * In case of in-process agents there is one component data object
     * per agent instance thread.
     * This method provides valid results even when called in the agent ctor.
     * @since 4.6
     */
    static KComponentData componentData();

Q_SIGNALS:
    /**
     * This signal is emitted whenever the name of the agent has changed.
     *
     * @param name The new name of the agent.
     *
     * @since 4.3
     */
    void agentNameChanged(const QString &name);

    /**
     * This signal should be emitted whenever the status of the agent has been changed.
     * @param status The new Status code.
     * @param message A i18n'ed description of the new status.
     */
    void status(int status, const QString &message = QString());

    /**
     * This signal should be emitted whenever the progress of an action in the agent
     * (e.g. data transfer, connection establishment to remote server etc.) has changed.
     *
     * @param progress The progress of the action in percent.
     */
    void percent(int progress);

    /**
     * This signal shall be used to report warnings.
     *
     * @param message The i18n'ed warning message.
     */
    void warning(const QString &message);

    /**
     * This signal shall be used to report errors.
     *
     * @param message The i18n'ed error message.
     */
    void error(const QString &message);

    /**
     * This signal should be emitted whenever the status of the agent has been changed.
     * @param status The object that describes the status change.
     *
     * @since 4.6
     */
    void advancedStatus(const QVariantMap &status);

    /**
     * Emitted when another application has remotely asked the agent to abort
     * its current operation.
     * Connect to this signal if your agent supports abortion. After aborting
     * and cleaning up, agents should return to Idle status.
     *
     * @since 4.4
     */
    void abortRequested();

    /**
     * Emitted if another application has changed the agent's configuration remotely
     * and called AgentInstance::reconfigure().
     *
     * @since 4.2
     */
    void reloadConfiguration();

    /**
     * Emitted when the online state changed.
     * @param online The online state.
     * @since 4.2
     */
    void onlineChanged(bool online);

    /**
     * This signal is emitted whenever the user has accepted the configuration dialog.
     *
     * @note Implementors of agents/resources are responsible to emit this signal if
     *       the agent/resource reimplements configure().
     *
     * @since 4.4
     */
    void configurationDialogAccepted();

    /**
     * This signal is emitted whenever the user has rejected the configuration dialog.
     *
     * @note Implementors of agents/resources are responsible to emit this signal if
     *       the agent/resource reimplements configure().
     *
     * @since 4.4
     */
    void configurationDialogRejected();

protected:
    /**
     * Creates an agent base.
     *
     * @param id The instance id of the agent.
     */
    AgentBase(const QString &id);

    /**
     * Destroys the agent base.
     */
    ~AgentBase();

    /**
     * This method is called whenever the agent application is about to
     * quit.
     *
     * Reimplement this method to do session cleanup (e.g. disconnecting
     * from groupware server).
     */
    virtual void aboutToQuit();

    /**
     * Returns the Akonadi::ChangeRecorder object used for monitoring.
     * Use this to configure which parts you want to monitor.
     */
    ChangeRecorder *changeRecorder() const;

    /**
     * Returns the config object for this Agent.
     */
    KSharedConfigPtr config();

    /**
     * Marks the current change as processes and replays the next change if change
     * recording is enabled (noop otherwise). This method is called
     * from the default implementation of the change notification slots. While not
     * required when not using change recording, it is nevertheless recommended
     * to call this method when done with processing a change notification.
     */
    void changeProcessed();

    /**
     * Returns whether the agent is currently online.
     */
    bool isOnline() const;

    /**
     * Sets whether the agent needs network or not.
     *
     * @since 4.2
     * @todo use this in combination with Solid::Networking::Notifier to change
     *       the onLine status of the agent.
     * @param needsNetwork @c true if the agents needs network. Defaults to @c false
     */
    void setNeedsNetwork(bool needsNetwork);

    /**
     * Sets whether the agent shall be online or not.
     */
    void setOnline(bool state);

protected:
    /**
      * Sets the agent offline but will make it online again after a given time
      *
      * Use this method when the agent detects some problem with its backend but it wants
      * to retry all pending operations after some time - e.g. a server can not be reached currently
      *
      * Example usage:
      * @code
      * void ExampleResource::onItemRemovedFinished(KJob *job)
      * {
      *     if (job->error()) {
      *         emit status(Broken, job->errorString());
      *         deferTask();
      *         setTemporaryOffline(300);
      *         return;
      *     }
      *     ...
      * }
      * @endcode
      *
      * @since 4.13
      * @param makeOnlineInSeconds timeout in seconds after which the agent changes to online
      */
    void setTemporaryOffline(int makeOnlineInSeconds = 300);

    //@cond PRIVATE
    AgentBasePrivate *d_ptr;
    explicit AgentBase(AgentBasePrivate *d, const QString &id);
    friend class ObserverV2;
    //@endcond

    /**
     * This method is called whenever the @p online status has changed.
     * Reimplement this method to react on online status changes.
     * @param online online status
     */
    virtual void doSetOnline(bool online);

private:
    //@cond PRIVATE
    static QString parseArguments(int, char **);
    static int init(AgentBase *r);
    void setOnlineInternal(bool state);

    // D-Bus interface stuff
    void abort();
    void reconfigure();
    void quit();

    // dbus agent interface
    friend class ::Akonadi__StatusAdaptor;
    friend class ::Akonadi__ControlAdaptor;

    Q_DECLARE_PRIVATE(AgentBase)
    Q_PRIVATE_SLOT(d_func(), void delayedInit())
    Q_PRIVATE_SLOT(d_func(), void slotStatus(int, const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotPercent(int))
    Q_PRIVATE_SLOT(d_func(), void slotWarning(const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotError(const QString &))
    Q_PRIVATE_SLOT(d_func(), void slotNetworkStatusChange(Solid::Networking::Status))
    Q_PRIVATE_SLOT(d_func(), void slotResumedFromSuspend())
    Q_PRIVATE_SLOT(d_func(), void slotTemporaryOfflineTimeout())

    //@endcond
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
