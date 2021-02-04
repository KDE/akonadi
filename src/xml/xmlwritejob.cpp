/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xmlwritejob.h"
#include "xmldocument.h"
#include "xmlwriter.h"

#include "collectionfetchjob.h"
#include "item.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QDebug>

#include <QDomElement>
#include <QStack>

using namespace Akonadi;

namespace Akonadi
{
class XmlWriteJobPrivate
{
public:
    explicit XmlWriteJobPrivate(XmlWriteJob *parent)
        : q(parent)
    {
    }

    XmlWriteJob *const q;
    Collection::List roots;
    QStack<Collection::List> pendingSiblings;
    QStack<QDomElement> elementStack;
    QString fileName;
    XmlDocument document;

    void collectionFetchResult(KJob *job);
    void processCollection();
    void itemFetchResult(KJob *job);
    void processItems();
};

} // namespace Akonadi

void XmlWriteJobPrivate::collectionFetchResult(KJob *job)
{
    if (job->error()) {
        return;
    }
    auto fetch = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetch);
    if (fetch->collections().isEmpty()) {
        processItems();
    } else {
        pendingSiblings.push(fetch->collections());
        processCollection();
    }
}

void XmlWriteJobPrivate::processCollection()
{
    if (!pendingSiblings.isEmpty() && pendingSiblings.top().isEmpty()) {
        pendingSiblings.pop();
        if (pendingSiblings.isEmpty()) {
            q->done();
            return;
        }
        processItems();
        return;
    }

    if (pendingSiblings.isEmpty()) {
        q->done();
        return;
    }

    const Collection current = pendingSiblings.top().first();
    qDebug() << "Writing " << current.name() << "into" << elementStack.top().attribute(QStringLiteral("name"));
    elementStack.push(XmlWriter::writeCollection(current, elementStack.top()));
    auto subfetch = new CollectionFetchJob(current, CollectionFetchJob::FirstLevel, q);
    q->connect(subfetch, &CollectionFetchJob::result, q, [this](KJob *job) {
        collectionFetchResult(job);
    });
}

void XmlWriteJobPrivate::processItems()
{
    const Collection collection = pendingSiblings.top().first();
    auto fetch = new ItemFetchJob(collection, q);
    fetch->fetchScope().fetchAllAttributes();
    fetch->fetchScope().fetchFullPayload();
    q->connect(fetch, &ItemFetchJob::result, q, [this](KJob *job) {
        itemFetchResult(job);
    });
}

void XmlWriteJobPrivate::itemFetchResult(KJob *job)
{
    if (job->error()) {
        return;
    }
    auto fetch = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetch);
    const Akonadi::Item::List lstItems = fetch->items();
    for (const Item &item : lstItems) {
        XmlWriter::writeItem(item, elementStack.top());
    }
    pendingSiblings.top().removeFirst();
    elementStack.pop();
    processCollection();
}

XmlWriteJob::XmlWriteJob(const Collection &root, const QString &fileName, QObject *parent)
    : Job(parent)
    , d(new XmlWriteJobPrivate(this))
{
    d->roots.append(root);
    d->fileName = fileName;
}

XmlWriteJob::XmlWriteJob(const Collection::List &roots, const QString &fileName, QObject *parent)
    : Job(parent)
    , d(new XmlWriteJobPrivate(this))
{
    d->roots = roots;
    d->fileName = fileName;
}

XmlWriteJob::~XmlWriteJob()
{
    delete d;
}

void XmlWriteJob::doStart()
{
    d->elementStack.push(d->document.document().documentElement());
    auto job = new CollectionFetchJob(d->roots, this);
    connect(job, &CollectionFetchJob::result, this, [this](KJob *job) {
        d->collectionFetchResult(job);
    });
}

void XmlWriteJob::done() // cannot be in the private class due to emitResult()
{
    if (!d->document.writeToFile(d->fileName)) {
        setError(Unknown);
        setErrorText(d->document.lastError());
    }
    emitResult();
}

#include "moc_xmlwritejob.cpp"
