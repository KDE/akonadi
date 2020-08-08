/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionpathresolver.h"
#include "interface.h"

#include "job_p.h"

#include "akonadicore_debug.h"

#include <KLocalizedString>

#include <QStringList>

using namespace Akonadi;

//@cond PRIVATE

class Akonadi::CollectionPathResolverPrivate
{
public:
    explicit CollectionPathResolverPrivate(CollectionPathResolver *parent)
        : q(parent)
    {}

    void init(const QString &path, const Collection &rootCollection)
    {
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

    void collectionsRetrieved(const Collection::List &cols);

    void collectionFetchError(const Error &error)
    {
        qCWarning(AKONADICORE_LOG) << "Failed to fetch Collections:" << error;
        q->setError(error.code());
        q->setErrorText(error.message());
        q->emitResult();
    }

    QStringList splitPath(const QString &path)
    {
        if (path.isEmpty()) {   // path is normalized, so non-empty means at least one hit
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

    Collection mCurrentNode;
    QStringList mPathParts;
    QString mPath;
    Collection::Id mColId = -1;
    bool mPathToId = false;

private:
    CollectionPathResolver * const q;
};

void CollectionPathResolverPrivate::collectionsRetrieved(const Collection::List &cols)
{
    if (cols.isEmpty()) {
        mColId = -1;
        q->setError(KJob::UserDefinedError);
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
            qCWarning(AKONADICORE_LOG) <<  "No such collection" << currentPart << "with parent" << mCurrentNode.id();
            mColId = -1;
            q->setError(KJob::UserDefinedError);
            q->setErrorText(i18n("No such collection."));
            q->emitResult();
            return;
        }
        if (mPathParts.isEmpty()) {
            mColId = mCurrentNode.id();
            q->emitResult();
            return;
        }

        Akonadi::fetchSubcollections(mCurrentNode).then(
                [this](const Collection::List &cols) {
                    collectionsRetrieved(cols);
                },
                [this](const Error &error) {
                    collectionFetchError(error);
                });
    } else {
        Collection col = cols.at(0);
        mCurrentNode = col.parentCollection();
        mPathParts.prepend(col.name());
        if (mCurrentNode == Collection::root()) {
            q->emitResult();
            return;
        }
        Akonadi::fetchCollection(mCurrentNode).then(
                [this](const Collection &col) {
                    collectionsRetrieved({col});
                },
                [this](const Error &error) {
                    collectionFetchError(error);
                });
    }
}

CollectionPathResolver::CollectionPathResolver(const QString &path, QObject *parent)
    : KJob(parent)
    , d(std::make_unique<CollectionPathResolverPrivate>(this))
{
    d->init(path, Collection::root());
}

CollectionPathResolver::CollectionPathResolver(const QString &path, const Collection &parentCollection, QObject *parent)
    : KJob(parent)
    , d(std::make_unique<CollectionPathResolverPrivate>(this))
{
    d->init(path, parentCollection);
}

CollectionPathResolver::CollectionPathResolver(const Collection &collection, QObject *parent)
    : KJob(parent)
    , d(std::make_unique<CollectionPathResolverPrivate>(this))
{
    d->mPathToId = false;
    d->mColId = collection.id();
    d->mCurrentNode = collection;
}

CollectionPathResolver::~CollectionPathResolver() = default;

Collection::Id CollectionPathResolver::collection() const
{
    return d->mColId;
}

QString CollectionPathResolver::path() const
{
    if (d->mPathToId) {
        return d->mPath;
    }
    return d->mPathParts.join(pathDelimiter());
}

QString CollectionPathResolver::pathDelimiter()
{
    return QStringLiteral("/");
}

void CollectionPathResolver::start()
{
    if (d->mPathToId) {
        if (d->mPath.isEmpty()) {
            d->mColId = Collection::root().id();
            emitResult();
            return;
        }
        Akonadi::fetchSubcollections(d->mCurrentNode).then(
                [this](const Collection::List &collections) {
                    d->collectionsRetrieved(collections);
                },
                [this](const Error &error) {
                    d->collectionFetchError(error);
                });
    } else {
        if (d->mColId == 0) {
            d->mColId = Collection::root().id();
            emitResult();
            return;
        }
        Akonadi::fetchCollection(d->mCurrentNode).then(
                [this](const Collection &collection) {
                    d->collectionsRetrieved({collection});
                },
                [this](const Error &error) {
                    d->collectionFetchError(error);
                });
    }
}

//@endcond

#include "moc_collectionpathresolver.cpp"
