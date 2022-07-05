/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "clearcachejob.h"
#include "akonadiwidgets_debug.h"
#include "dbaccess.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi;
ClearCacheJob::ClearCacheJob(QObject *parent)
    : QObject{parent}
{
}

ClearCacheJob::~ClearCacheJob() = default;

const Akonadi::Collection &ClearCacheJob::collection() const
{
    return mCollection;
}

void ClearCacheJob::setCollection(const Akonadi::Collection &newCollection)
{
    mCollection = newCollection;
}

void ClearCacheJob::start()
{
    if (!canStart()) {
        deleteLater();
        return;
    }

    const auto ridCount = QStringLiteral("SELECT COUNT(*) FROM PimItemTable WHERE collectionId=%1 AND remoteId=''").arg(mCollection.id());
    QSqlQuery query(DbAccess::database());
    if (!query.exec(ridCount)) {
        qCWarning(AKONADIWIDGETS_LOG) << "Failed to execute query" << ridCount << ":" << query.lastError().text();
        KMessageBox::error(mParentWidget, query.lastError().text());
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
                                                    QString::number(mCollection.id()),
                                                    QString::number(emptyRidCount)),
                                               QStringLiteral("POSSIBLE DATA LOSS!"))
            == KMessageBox::Cancel) {
            return;
        }
    }

    const QString str = QStringLiteral("DELETE FROM PimItemTable WHERE collectionId=%1").arg(mCollection.id());
    qCWarning(AKONADIWIDGETS_LOG) << str;
    query = QSqlQuery(str, DbAccess::database());
    if (query.exec()) {
        if (query.lastError().isValid()) {
            qCWarning(AKONADIWIDGETS_LOG) << query.lastError();
            KMessageBox::error(mParentWidget, query.lastError().text());
        }
    }

    // TODO: Clear external parts
    // TODO: Reset Akonadi's internal collection statistics cache
    // TODO: Notify all clients EXCEPT FOR THE RESOURCE about the deletion?
    // TODO: Clear search index
    // TODO: ???

    KMessageBox::information(mParentWidget, i18n("Collection cache cleared. You should restart Akonadi now."));
}

bool ClearCacheJob::canStart() const
{
    return mCollection.isValid();
}

QWidget *ClearCacheJob::parentWidget() const
{
    return mParentWidget;
}

void ClearCacheJob::setParentWidget(QWidget *newParentWidget)
{
    mParentWidget = newParentWidget;
}
