/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2022-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "akonadiwidgets_export.h"
#include <Akonadi/Collection>
#include <QObject>
namespace Akonadi
{
class AKONADIWIDGETS_EXPORT ClearCacheFoldersJob : public QObject
{
    Q_OBJECT
public:
    explicit ClearCacheFoldersJob(const Akonadi::Collection &folder, QObject *parent = nullptr);
    explicit ClearCacheFoldersJob(const Akonadi::Collection::List &folders, QObject *parent = nullptr);
    ~ClearCacheFoldersJob() override;

    void start();

    [[nodiscard]] bool canStart() const;

    [[nodiscard]] QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

    [[nodiscard]] bool canceled() const;
    void setCanceled(bool newCanceled);

Q_SIGNALS:
    void clearCacheDone();
    void clearNextFolder();
    void finished(bool success);

private:
    void slotClearNextFolder();
    Akonadi::Collection::List mCollections;
    QWidget *mParentWidget = nullptr;
    int mNumberJob = 0;
    bool mCanceled = false;
};
}
