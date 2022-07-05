/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "akonadiwidgets_export.h"
#include <Akonadi/Collection>
#include <QObject>
namespace Akonadi
{
class AKONADIWIDGETS_EXPORT ClearCacheJob : public QObject
{
    Q_OBJECT
public:
    explicit ClearCacheJob(QObject *parent = nullptr);
    ~ClearCacheJob() override;

    Q_REQUIRED_RESULT const Akonadi::Collection &collection() const;
    void setCollection(const Akonadi::Collection &newCollection);

    void start();

    Q_REQUIRED_RESULT bool canStart() const;

    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

private:
    Akonadi::Collection mCollection;
    QWidget *mParentWidget = nullptr;
};
}
