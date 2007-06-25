/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#ifndef AKONADI_RESOURCE_H
#define AKONADI_RESOURCE_H

#include "libakonadi_export.h"
#include <QtCore/QObject>

#ifndef Q_NOREPLY
#define Q_NOREPLY
#endif


namespace Akonadi {

/**
 * Abstract interface for all resource agent classes.
 * You should however use @see ResourceBase as base class, as it
 * provides a lot of convenience methods and abstracts parts
 * of the protocol.
 */
class AKONADI_EXPORT Resource : public QObject
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Resource" )

  public:
    typedef QList<Resource*> List;

    /**
     * Destroys the resource.
     */
    virtual ~Resource() { }

  public Q_SLOTS:

    /**
     * This method is called to quit the resource.
     */
    virtual void quit() = 0;

    /**
     * This method returns the current status code of the resource.
     *
     * The following return values are possible:
     *
     *  0 - Ready
     *  1 - Syncing
     *  2 - Error
     */
    virtual int status() const = 0;

    /**
     * This method returns an i18n'ed description of the current status code.
     */
    virtual QString statusMessage() const = 0;

    /**
     * This method returns the current progress of the resource in percentage.
     */
    virtual uint progress() const = 0;

    /**
     * This method returns an i18n'ed description of the current progress.
     */
    virtual QString progressMessage() const = 0;

    /**
     * This method is called whenever an external query for putting data in the
     * storage is received.
     *
     * @param uid The Akonadi uid of the item that is requested.
     * @param remoteId The remote identifier of the item that is requested.
     * @param parts The item pasrts that should be fetched.
     */
    virtual bool requestItemDelivery( int uid, const QString &remoteId, const QStringList &parts ) = 0;

    /**
     * This method is called whenever the resource shall show its configuration dialog
     * to the user.
     */
    virtual Q_NOREPLY void configure() = 0;

    /**
     * This method is called whenever the resource should start synchronization.
     */
    virtual Q_NOREPLY void synchronize() = 0;

    /**
     * Synchronize the given collection.
     * @param collectionId The identifier of the collection to synchronize.
     */
    virtual Q_NOREPLY void synchronizeCollection( int collectionId ) = 0;

    /**
     * This method is used to set the name of the resource.
     */
    virtual void setName( const QString &name ) = 0;

    /**
     * Returns the name of the resource.
     */
    virtual QString name() const = 0;

    /**
     * This method is called when the resource is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup() const = 0;

    /**
     * Returns true if the resource is in online mode.
     */
    virtual bool isOnline() const = 0;

    /**
     * Sets the online/offline mode. In offline mode, the resource
     * should not do any network operations and instead record all changes
     * locally until switched back into online mode.
     * @param state Switch to online mode if true, to offline mode otherwise.
     */
    virtual void setOnline( bool state ) = 0;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the status of the resource has changed.
     *
     * @param status The status id of the resource (@see Status).
     * @param message An i18n'ed message which describes the status in detail.
     */
    void statusChanged( int status, const QString &message );

    /**
     * This signal is emitted whenever the progress information of the resource
     * has changed.
     *
     * @param progress The progress in percent (0 - 100).
     * @param message An i18n'ed message which describes the progress in detail.
     */
    void progressChanged( uint progress, const QString &message );

    /**
     * This signal is emitted whenever the name of the resource has changed.
     *
     * @param name The new name of the resource.
     */
    void nameChanged( const QString &name );
};

}

#endif
