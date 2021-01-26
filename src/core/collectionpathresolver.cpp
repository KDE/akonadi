/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionpathresolver.h"

#include "collectionfetchjob.h"
#include "job_p.h"

#include "akonadicore_debug.h"

#include <KLocalizedString>

#include <QStringList>

using namespace Akonadi;

//@cond PRIVATE

class Akonadi::CollectionPathResolverPrivate : public JobPrivate
{
public:
    explicit CollectionPathResolverPrivate(CollectionPathResolver *parent)
        : JobPrivate(parent)
        , mColId(-1)
    {
    }

    void init(const QString &path, const Collection &rootCollection)
    {
        Q_Q(CollectionPathResolver);

        mPathToId = true;
        mPath = path;
        if (mPath.startsWith(q->pathDelimiter())) {
            mPath = mPath.right(mPath.length() - q->pathDelimiter().length());
        }
        if (mPath.endsWith(q->pathDelimiter())) {
            mPath = mPath.left(mPath.length() - q->pathDelimiter().length());
        }

        mPathParts = splitPath(mPath);
        mCurrentNode = rootCollection;
    }

    void jobResult(KJob *job);

    QStringList splitPath(const QString &path)
    {
        if (path.isEmpty()) { // path is normalized, so non-empty means at least one hit
            return QStringList();
        }

        QStringList rv;
        int begin = 0;
        const int pathSize(path.size());
        for (int i = 0; i < pathSize; ++i) {
            if (path[i] == QLatin1Char('/')) {
                QString pathElement = path.mid(begin, i - begin);
                pathElement.replace(QLatin1String("\\/"), QLatin1String("/"));
                rv.append(pathElement);
                begin = i + 1;
            }
            if (i < path.size() - 2 && path[i] == QLatin1Char('\\') && path[i + 1] == QLatin1Char('/')) {
                ++i;
            }
        }
        QString pathElement = path.mid(begin);
        pathElement.replace(QLatin1String("\\/"), QLatin1String("/"));
        rv.append(pathElement);
        return rv;
    }

    Q_DECLARE_PUBLIC(CollectionPathResolver)

    Collection mCurrentNode;
    QStringList mPathParts;
    QString mPath;
    Collection::Id mColId;
    bool mPathToId = false;
};

void CollectionPathResolverPrivate::jobResult(KJob *job)
{
    if (job->error()) {
        return;
    }

    Q_Q(CollectionPathResolver);

    auto *list = static_cast<CollectionFetchJob *>(job);
    CollectionFetchJob *nextJob = nullptr;
    const Collection::List cols = list->collections();
    if (cols.isEmpty()) {
        mColId = -1;
        q->setError(CollectionPathResolver::Unknown);
        q->setErrorText(i18n("No such collection."));
        q->emitResult();
        return;
    }

    if (mPathToId) {
        const QString currentPart = mPathParts.takeFirst();
        bool found = false;
        for (const Collection &c : cols) {
            if (c.name() == currentPart) {
                mCurrentNode = c;
                found = true;
                break;
            }
        }
        if (!found) {
            qCWarning(AKONADICORE_LOG) << "No such collection" << currentPart << "with parent" << mCurrentNode.id();
            mColId = -1;
            q->setError(CollectionPathResolver::Unknown);
            q->setErrorText(i18n("No such collection."));
            q->emitResult();
            return;
        }
        if (mPathParts.isEmpty()) {
            mColId = mCurrentNode.id();
            q->emitResult();
            return;
        }
        nextJob = new CollectionFetchJob(mCurrentNode, CollectionFetchJob::FirstLevel, q);
    } else {
        Collection col = list->collections().at(0);
        mCurrentNode = col.parentCollection();
        mPathParts.prepend(col.name());
        if (mCurrentNode == Collection::root()) {
            q->emitResult();
            return;
        }
        nextJob = new CollectionFetchJob(mCurrentNode, CollectionFetchJob::Base, q);
    }
    q->connect(nextJob, &CollectionFetchJob::result, q, [this](KJob *job) {
        jobResult(job);
    });
}

CollectionPathResolver::CollectionPathResolver(const QString &path, QObject *parent)
    : Job(new CollectionPathResolverPrivate(this), parent)
{
    Q_D(CollectionPathResolver);
    d->init(path, Collection::root());
}

CollectionPathResolver::CollectionPathResolver(const QString &path, const Collection &parentCollection, QObject *parent)
    : Job(new CollectionPathResolverPrivate(this), parent)
{
    Q_D(CollectionPathResolver);
    d->init(path, parentCollection);
}

CollectionPathResolver::CollectionPathResolver(const Collection &collection, QObject *parent)
    : Job(new CollectionPathResolverPrivate(this), parent)
{
    Q_D(CollectionPathResolver);

    d->mPathToId = false;
    d->mColId = collection.id();
    d->mCurrentNode = collection;
}

CollectionPathResolver::~CollectionPathResolver()
{
}

Collection::Id CollectionPathResolver::collection() const
{
    Q_D(const CollectionPathResolver);

    return d->mColId;
}

QString CollectionPathResolver::path() const
{
    Q_D(const CollectionPathResolver);

    if (d->mPathToId) {
        return d->mPath;
    }
    return d->mPathParts.join(pathDelimiter());
}

QString CollectionPathResolver::pathDelimiter()
{
    return QStringLiteral("/");
}

void CollectionPathResolver::doStart()
{
    Q_D(CollectionPathResolver);

    CollectionFetchJob *job = nullptr;
    if (d->mPathToId) {
        if (d->mPath.isEmpty()) {
            d->mColId = Collection::root().id();
            emitResult();
            return;
        }
        job = new CollectionFetchJob(d->mCurrentNode, CollectionFetchJob::FirstLevel, this);
    } else {
        if (d->mColId == 0) {
            d->mColId = Collection::root().id();
            emitResult();
            return;
        }
        job = new CollectionFetchJob(d->mCurrentNode, CollectionFetchJob::Base, this);
    }
    connect(job, &CollectionFetchJob::result, this, [d](KJob *job) {
        d->jobResult(job);
    });
}

//@endcond

#include "moc_collectionpathresolver.cpp"
