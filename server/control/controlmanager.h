/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QtCore/QObject>

/**
 * The control manager provides a dbus method to shutdown
 * the Akonadi Control process cleanly.
 */
class ControlManager : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new control manager.
     */
    ControlManager( QObject *parent = 0 );

    /**
     * Destroys the control manager.
     */
    ~ControlManager();

  public Q_SLOTS:
    /**
     * Shutdown the Akonadi Control process cleanly.
     */
    void shutdown();
};

#endif
