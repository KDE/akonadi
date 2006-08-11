/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#ifndef PROFILEVIEW_H
#define PROFILEVIEW_H

#include <QtGui/QWidget>

namespace PIM {

/**
 * This class provides a view of all available profiles.
 *
 * Since the view is listening to the dbus for changes, the
 * view is updated automatically as soon as a new profile
 * is created or removed to/from the profile manager.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ProfileView : public QWidget
{
  public:
    /**
     * Creates a new profile view.
     *
     * @param parent The parent widget.
     */
    ProfileView( QWidget *parent = 0 );

    /**
     * Destroys the profile view.
     */
    ~ProfileView();

    /**
     * Returns the currently selected profile or an
     * empty string if no profile is selected.
     */
    QString currentProfile() const;

  private:
    class Private;
    Private* const d;
};

}

#endif
