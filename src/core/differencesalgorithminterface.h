/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

namespace Akonadi
{
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
    virtual void compare(AbstractDifferencesReporter *reporter, const Akonadi::Item &leftItem, const Akonadi::Item &rightItem) = 0;
};

}

Q_DECLARE_INTERFACE(Akonadi::DifferencesAlgorithmInterface, "org.freedesktop.Akonadi.DifferencesAlgorithmInterface/1.0")

