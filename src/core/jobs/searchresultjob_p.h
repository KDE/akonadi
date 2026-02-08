/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class SearchResultJobPrivate;
class Collection;

/*!
 * \class Akonadi::SearchResultJob
 * \inheaderfile Akonadi/SearchResultJob
 * \inmodule AkonadiCore
 *
 * \internal
 */
class AKONADICORE_EXPORT SearchResultJob : public Akonadi::Job
{
    Q_OBJECT
public:
    explicit SearchResultJob(const QByteArray &searchId, const Collection &collection, QObject *parent = nullptr);
    ~SearchResultJob() override;

    void setSearchId(const QByteArray &searchId);
    [[nodiscard]] QByteArray searchId() const;

    void setResult(const QList<QByteArray> &remoteIds);
    void setResult(const QList<qint64> &ids);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(SearchResultJob)
};
}
