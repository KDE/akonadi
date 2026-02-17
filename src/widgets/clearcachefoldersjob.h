/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2022-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "akonadiwidgets_export.h"
#include <Akonadi/Collection>
#include <QObject>
namespace Akonadi
{
/*!
 * \brief The ClearCacheFoldersJob class
 *
 * \class Akonadi::ClearCacheFoldersJob
 * \inheaderfile Akonadi/ClearCacheFoldersJob
 * \inmodule AkonadiWidgets
 */
class AKONADIWIDGETS_EXPORT ClearCacheFoldersJob : public QObject
{
    Q_OBJECT
public:
    /*!
     * Creates a job to clear the cache for a single folder.
     * \a folder The collection folder whose cache should be cleared.
     * \a parent The parent object.
     */
    explicit ClearCacheFoldersJob(const Akonadi::Collection &folder, QObject *parent = nullptr);
    /*!
     * Creates a job to clear the cache for multiple folders.
     * \a folders The collection folders whose cache should be cleared.
     * \a parent The parent object.
     */
    explicit ClearCacheFoldersJob(const Akonadi::Collection::List &folders, QObject *parent = nullptr);
    /*!
     * Destroys the clear cache folders job.
     */
    ~ClearCacheFoldersJob() override;

    /*!
     * Starts the cache clearing operation.
     */
    void start();

    /*!
     * Returns whether the job can be started.
     * \return True if the job can be started, false otherwise.
     */
    [[nodiscard]] bool canStart() const;

    /*!
     * Returns the parent widget for this job.
     * \return The parent widget, or nullptr if none was set.
     */
    [[nodiscard]] QWidget *parentWidget() const;
    /*!
     * Sets the parent widget for this job.
     * \a newParentWidget The parent widget.
     */
    void setParentWidget(QWidget *newParentWidget);

    /*!
     * Returns whether the job was canceled.
     * \return True if canceled, false otherwise.
     */
    [[nodiscard]] bool canceled() const;
    /*!
     * Sets the canceled state of this job.
     * \a newCanceled True to cancel the job.
     */
    void setCanceled(bool newCanceled);

Q_SIGNALS:
    /*!
     * Emitted when the cache clearing is done.
     */
    void clearCacheDone();
    /*!
     * Emitted when the next folder's cache is about to be cleared.
     */
    void clearNextFolder();
    /*!
     * Emitted when the operation has finished.
     * \a success True if the operation was successful, false otherwise.
     */
    void finished(bool success);

private:
    void slotClearNextFolder();
    Akonadi::Collection::List mCollections;
    QWidget *mParentWidget = nullptr;
    int mNumberJob = 0;
    bool mCanceled = false;
};
}
