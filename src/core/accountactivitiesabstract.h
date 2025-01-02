/*
    SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include <QObject>
namespace Akonadi
{
class AKONADICORE_EXPORT AccountActivitiesAbstract : public QObject
{
    Q_OBJECT
public:
    explicit AccountActivitiesAbstract(QObject *parent = nullptr);
    ~AccountActivitiesAbstract() override;

    [[nodiscard]] virtual bool filterAcceptsRow(const QStringList &activities) const = 0;

    [[nodiscard]] virtual bool hasActivitySupport() const = 0;

    [[nodiscard]] virtual QString currentActivity() const = 0;
Q_SIGNALS:
    void activitiesChanged();
};
}
