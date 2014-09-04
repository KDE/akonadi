/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef DIFFERENCESALGORITHMINTERFACE_P_H
#define DIFFERENCESALGORITHMINTERFACE_P_H

#include <QtCore/QObject>

namespace Akonadi {

class AbstractDifferencesReporter;
class Item;

/**
 * @short An interface to find out differences between two Akonadi objects.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class DifferencesAlgorithmInterface
{
public:
    /**
     * Destroys the differences algorithm interface.
     */
    virtual ~DifferencesAlgorithmInterface()
    {
    }

    /**
     * Calculates the differences between two Akonadi objects and reports
     * them to a reporter object.
     *
     * @param reporter The reporter object that will be used for reporting the differences.
     * @param leftItem The left-hand side item that will be compared.
     * @param rightItem The right-hand side item that will be compared.
     */
    virtual void compare(AbstractDifferencesReporter *reporter,
                         const Akonadi::Item &leftItem,
                         const Akonadi::Item &rightItem) = 0;
};

}

Q_DECLARE_INTERFACE(Akonadi::DifferencesAlgorithmInterface, "org.freedesktop.Akonadi.DifferencesAlgorithmInterface/1.0")

#endif
