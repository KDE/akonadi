/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "partfetcher.h"

#include "entitytreemodel.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "session.h"
#include <KLocalizedString>

Q_DECLARE_METATYPE(QSet<QByteArray>)

using namespace Akonadi;

namespace Akonadi
{
class PartFetcherPrivate
{
    PartFetcherPrivate(PartFetcher *partFetcher, const QModelIndex &index, const QByteArray &partName)
        : m_persistentIndex(index)
        , m_partName(partName)
        , q_ptr(partFetcher)
    {
    }

    void fetchJobDone(KJob *job);

    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    QPersistentModelIndex m_persistentIndex;
    QByteArray m_partName;
    Item m_item;

    Q_DECLARE_PUBLIC(PartFetcher)
    PartFetcher *q_ptr;
};

} // namespace Akonadi

void PartFetcherPrivate::fetchJobDone(KJob *job)
{
    Q_Q(PartFetcher);
    if (job->error()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to fetch item for index"));
        q->emitResult();
        return;
    }

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);

    const Item::List list = fetchJob->items();

    Q_ASSERT(list.size() == 1);

    // If m_persistentIndex comes from a selection proxy model, it could become
    // invalid if the user clicks around a lot.
    if (!m_persistentIndex.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Index is no longer available"));
        q->emitResult();
        return;
    }

    const auto loadedParts = m_persistentIndex.data(EntityTreeModel::LoadedPartsRole).value<QSet<QByteArray>>();

    Q_ASSERT(!loadedParts.contains(m_partName));

    Item item = m_persistentIndex.data(EntityTreeModel::ItemRole).value<Item>();

    item.apply(list.at(0));

    auto model = const_cast<QAbstractItemModel *>(m_persistentIndex.model());

    Q_ASSERT(model);

    QVariant itemVariant = QVariant::fromValue(item);
    model->setData(m_persistentIndex, itemVariant, EntityTreeModel::ItemRole);

    m_item = item;

    q->emitResult();
}

PartFetcher::PartFetcher(const QModelIndex &index, const QByteArray &partName, QObject *parent)
    : KJob(parent)
    , d_ptr(new PartFetcherPrivate(this, index, partName))
{
}

PartFetcher::~PartFetcher()
{
    delete d_ptr;
}

void PartFetcher::start()
{
    Q_D(PartFetcher);

    const QModelIndex index = d->m_persistentIndex;

    const auto loadedParts = index.data(EntityTreeModel::LoadedPartsRole).value<QSet<QByteArray>>();

    if (loadedParts.contains(d->m_partName)) {
        d->m_item = d->m_persistentIndex.data(EntityTreeModel::ItemRole).value<Item>();
        emitResult();
        return;
    }

    const auto availableParts = index.data(EntityTreeModel::AvailablePartsRole).value<QSet<QByteArray>>();
    if (!availableParts.contains(d->m_partName)) {
        setError(UserDefinedError);
        setErrorText(i18n("Payload part '%1' is not available for this index", QString::fromLatin1(d->m_partName)));
        emitResult();
        return;
    }

    auto session = qobject_cast<Akonadi::Session *>(qvariant_cast<QObject *>(index.data(EntityTreeModel::SessionRole)));

    if (!session) {
        setError(UserDefinedError);
        setErrorText(i18n("No session available for this index"));
        emitResult();
        return;
    }

    const auto item = index.data(EntityTreeModel::ItemRole).value<Akonadi::Item>();

    if (!item.isValid()) {
        setError(UserDefinedError);
        setErrorText(i18n("No item available for this index"));
        emitResult();
        return;
    }

    ItemFetchScope scope;
    scope.fetchPayloadPart(d->m_partName);
    auto itemFetchJob = new Akonadi::ItemFetchJob(item, session);
    itemFetchJob->setFetchScope(scope);
    connect(itemFetchJob, &KJob::result, this, [d](KJob *job) {
        d->fetchJobDone(job);
    });
}

QModelIndex PartFetcher::index() const
{
    Q_D(const PartFetcher);

    return d->m_persistentIndex;
}

QByteArray PartFetcher::partName() const
{
    Q_D(const PartFetcher);

    return d->m_partName;
}

Item PartFetcher::item() const
{
    Q_D(const PartFetcher);

    return d->m_item;
}

#include "moc_partfetcher.cpp"
