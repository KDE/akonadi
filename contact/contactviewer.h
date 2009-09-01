/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTVIEWER_H
#define AKONADI_CONTACTVIEWER_H

#include "akonadi-contact_export.h"

#include <akonadi/itemmonitor.h>

#include <QtGui/QWidget>

namespace Akonadi {

/**
 * @short A viewer component for contacts in Akonadi.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT ContactViewer : public QWidget, public Akonadi::ItemMonitor
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact viewer.
     *
     * @param parent The parent widget.
     */
    ContactViewer( QWidget *parent = 0 );

    /**
     * Destroys the contact viewer.
     */
    ~ContactViewer();

    /**
     * Returns the contact that is currently displayed.
     */
    Akonadi::Item contact() const;

  public Q_SLOTS:
    /**
     * Sets the @p contact that shall be displayed in the viewer.
     */
    void setContact( const Akonadi::Item &contact );

  private:
    /**
     * This method is called whenever the displayed contact has been changed.
     */
    virtual void itemChanged( const Item &contact );

    /**
     * This method is called whenever the displayed contact has been
     * removed from Akonadi.
     */
    virtual void itemRemoved();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
