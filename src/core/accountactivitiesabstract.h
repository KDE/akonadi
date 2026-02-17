/*
    SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include <QObject>
namespace Akonadi
{
/*!
 * \class Akonadi::AccountActivitiesAbstract
 * \inheaderfile Akonadi/AccountActivitiesAbstract
 * \inmodule AkonadiCore
 *
 * \brief The AccountActivitiesAbstract class
 */
class AKONADICORE_EXPORT AccountActivitiesAbstract : public QObject
{
    Q_OBJECT
public:
    /*!
     * Constructs an AccountActivitiesAbstract object.
     * \a parent The parent object.
     */
    explicit AccountActivitiesAbstract(QObject *parent = nullptr);
    /*!
     * Destructor.
     */
    ~AccountActivitiesAbstract() override;

    /*!
     * Filters rows based on the given activities list.
     * \a activities A list of activity identifiers to filter against.
     * \return True if the row should be accepted, false otherwise.
     */
    [[nodiscard]] virtual bool filterAcceptsRow(const QStringList &activities) const = 0;

    /*!
     * Returns whether the account activities system is supported.
     * \return True if activity support is available, false otherwise.
     */
    [[nodiscard]] virtual bool hasActivitySupport() const = 0;

    /*!
     * Returns the identifier of the current activity.
     * \return The current activity identifier as a string.
     */
    [[nodiscard]] virtual QString currentActivity() const = 0;
Q_SIGNALS:
    /*!
     * Emitted when the list of available activities has changed.
     */
    void activitiesChanged();
};
}
