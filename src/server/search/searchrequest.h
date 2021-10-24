/*
    SPDX-FileCopyrightText: 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVector>

namespace Akonadi
{
namespace Server
{
class Connection;
class SearchManager;
class SearchTaskManager;

class SearchRequest : public QObject
{
    Q_OBJECT

public:
    explicit SearchRequest(const QByteArray &connectionId, SearchManager &searchManager, SearchTaskManager &agentSearchTask);
    ~SearchRequest() override;

    void setQuery(const QString &query);
    QString query() const;
    void setCollections(const QVector<qint64> &collections);
    QVector<qint64> collections() const;
    void setMimeTypes(const QStringList &mimeTypes);
    QStringList mimeTypes() const;
    void setRemoteSearch(bool remote);
    bool remoteSearch() const;

    /**
     * Whether results should be stored after they are emitted via resultsAvailable(),
     * so that they can be extracted via results() after the search is over. This
     * is disabled by default.
     */
    void setStoreResults(bool storeResults);

    QByteArray connectionId() const;

    void exec();

    QSet<qint64> results() const;

Q_SIGNALS:
    void resultsAvailable(const QSet<qint64> &results);

private:
    void searchPlugins();
    void emitResults(const QSet<qint64> &results);

    QByteArray mConnectionId;
    QString mQuery;
    QVector<qint64> mCollections;
    QStringList mMimeTypes;
    bool mRemoteSearch = false;
    bool mStoreResults = false;
    QSet<qint64> mResults;

    SearchManager &mSearchManager;
    SearchTaskManager &mAgentSearchManager;
};

} // namespace Server
} // namespace Akonadi

