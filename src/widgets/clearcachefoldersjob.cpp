/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2022-2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "clearcachefoldersjob.h"
#include "akonadiwidgets_debug.h"
#include "dbaccess.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi;
ClearCacheFoldersJob::ClearCacheFoldersJob(const Akonadi::Collection &folder, QObject *parent)
    : ClearCacheFoldersJob(Akonadi::Collection::List{folder}, parent)
{
}

ClearCacheFoldersJob::ClearCacheFoldersJob(const Akonadi::Collection::List &folders, QObject *parent)
    : QObject{parent}
{
    mCollections = folders;
    mNumberJob = folders.length();
    connect(this, &ClearCacheFoldersJob::clearNextFolder, this, &ClearCacheFoldersJob::slotClearNextFolder);
}

ClearCacheFoldersJob::~ClearCacheFoldersJob() = default;

void ClearCacheFoldersJob::slotClearNextFolder()
{
    if (mCanceled) {
        Q_EMIT clearCacheDone();
        Q_EMIT finished(false);
        deleteLater();
        return;
    }
    if (mNumberJob == -1) {
        KMessageBox::information(mParentWidget, i18n("Collection cache cleared. You should restart Akonadi now."));
        Q_EMIT clearCacheDone();
        Q_EMIT finished(true);
        deleteLater();
        return;
    }
    if (!mCollections.at(mNumberJob).isValid()) {
        Q_EMIT clearCacheDone();
        Q_EMIT finished(false);
        deleteLater();
        return;
    }

    const auto akonadiId = mCollections.at(mNumberJob).id();
    const auto ridCount = QStringLiteral("SELECT COUNT(*) FROM PimItemTable WHERE collectionId=%1 AND remoteId=''").arg(akonadiId);
    QSqlQuery query(DbAccess::database());
    if (!query.exec(ridCount)) {
        qCWarning(AKONADIWIDGETS_LOG) << "Failed to execute query" << ridCount << ":" << query.lastError().text();
        KMessageBox::error(mParentWidget, query.lastError().text());
        Q_EMIT finished(false);
        deleteLater();
        return;
    }

    query.next();
    const int emptyRidCount = query.value(0).toInt();
    if (emptyRidCount > 0) {
        if (KMessageBox::warningContinueCancel(mParentWidget,
                                               i18n("The collection '%1' contains %2 items without Remote ID. "
                                                    "Those items were likely never uploaded to the destination server, "
                                                    "so clearing this collection means that those <b>data will be lost</b>. "
                                                    "Are you sure you want to proceed?",
                                                    QString::number(akonadiId),
                                                    QString::number(emptyRidCount)),
                                               QStringLiteral("POSSIBLE DATA LOSS!"))
            == KMessageBox::Cancel) {
            Q_EMIT finished(false);
            deleteLater();
            return;
        }
    }

    const QString str = QStringLiteral("DELETE FROM PimItemTable WHERE collectionId=%1").arg(akonadiId);
    qCWarning(AKONADIWIDGETS_LOG) << str;
    query = QSqlQuery(str, DbAccess::database());
    if (query.exec()) {
        const auto lastError = query.lastError();
        if (lastError.isValid()) {
            qCWarning(AKONADIWIDGETS_LOG) << lastError;
            KMessageBox::error(mParentWidget, lastError.text());
        }
    }
    mNumberJob--;
    Q_EMIT clearNextFolder();
}

bool ClearCacheFoldersJob::canceled() const
{
    return mCanceled;
}

void ClearCacheFoldersJob::setCanceled(bool newCanceled)
{
    mCanceled = newCanceled;
}

void ClearCacheFoldersJob::start()
{
    if (!canStart()) {
        deleteLater();
        Q_EMIT finished(false);
        return;
    }
    mNumberJob--;
    Q_EMIT clearNextFolder();
}

bool ClearCacheFoldersJob::canStart() const
{
    return !mCollections.isEmpty();
}

QWidget *ClearCacheFoldersJob::parentWidget() const
{
    return mParentWidget;
}

void ClearCacheFoldersJob::setParentWidget(QWidget *newParentWidget)
{
    mParentWidget = newParentWidget;
}
