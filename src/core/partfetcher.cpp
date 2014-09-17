/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "partfetcher.h"

#include "entitytreemodel.h"
#include "session.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include <KLocalizedString>

Q_DECLARE_METATYPE(QSet<QByteArray>)

using namespace Akonadi;

namespace Akonadi {

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

}

void PartFetcherPrivate::fetchJobDone(KJob *job)
{
    Q_Q(PartFetcher);
    if (job->error()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to fetch item for index"));
        q->emitResult();
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);

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

    const QSet<QByteArray> loadedParts = m_persistentIndex.data(EntityTreeModel::LoadedPartsRole).value<QSet<QByteArray> >();

    Q_ASSERT(!loadedParts.contains(m_partName));

    Item item = m_persistentIndex.data(EntityTreeModel::ItemRole).value<Item>();

    item.apply(list.at(0));

    QAbstractItemModel *model = const_cast<QAbstractItemModel *>(m_persistentIndex.model());

    Q_ASSERT(model);

    QVariant itemVariant = QVariant::fromValue(item);
    model->setData(m_persistentIndex, itemVariant, EntityTreeModel::ItemRole);

    m_item = item;

    emit q->emitResult();
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

    const QSet<QByteArray> loadedParts = index.data(EntityTreeModel::LoadedPartsRole).value<QSet<QByteArray> >();

    if (loadedParts.contains(d->m_partName)) {
        d->m_item = d->m_persistentIndex.data(EntityTreeModel::ItemRole).value<Item>();
        emitResult();
        return;
    }

    const QSet<QByteArray> availableParts = index.data(EntityTreeModel::AvailablePartsRole).value<QSet<QByteArray> >();
    if (!availableParts.contains(d->m_partName)) {
        setError(UserDefinedError);
        setErrorText(i18n("Payload part '%1' is not available for this index", QString::fromLatin1(d->m_partName)));
        emitResult();
        return;
    }

    Akonadi::Session *session = qobject_cast<Akonadi::Session *>(qvariant_cast<QObject *>(index.data(EntityTreeModel::SessionRole)));

    if (!session) {
        setError(UserDefinedError);
        setErrorText(i18n("No session available for this index"));
        emitResult();
        return;
    }

    const Akonadi::Item item = index.data(EntityTreeModel::ItemRole).value<Akonadi::Item>();

    if (!item.isValid()) {
        setError(UserDefinedError);
        setErrorText(i18n("No item available for this index"));
        emitResult();
        return;
    }

    ItemFetchScope scope;
    scope.fetchPayloadPart(d->m_partName);
    ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(item, session);
    itemFetchJob->setFetchScope(scope);

    connect(itemFetchJob, SIGNAL(result(KJob*)),
            this, SLOT(fetchJobDone(KJob*)));
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
