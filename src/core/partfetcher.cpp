/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "partfetcher.h"

#include "entitytreemodel.h"
#include "session.h"
#include "itemfetchscope.h"
#include "interface.h"
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

    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    QPersistentModelIndex m_persistentIndex;
    QByteArray m_partName;
    Item m_item;

    Q_DECLARE_PUBLIC(PartFetcher)
    PartFetcher *q_ptr;
};

} // namespace Akonadi

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

    auto *session = qobject_cast<Akonadi::Session *>(qvariant_cast<QObject *>(index.data(EntityTreeModel::SessionRole)));

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

    ItemFetchOptions options;
    options.itemFetchScope().fetchPayloadPart(d->m_partName);
    Akonadi::fetchItem(item, options, session).then(
        [this](const Item &item) {
            Q_D(PartFetcher);
            // If m_persistentIndex comes from a selection proxy model, it could become
            // invalid if the user clicks around a lot.
            if (!d->m_persistentIndex.isValid()) {
                setError(KJob::UserDefinedError);
                setErrorText(i18n("Index is no longer available"));
                emitResult();
                return;
            }

            const QSet<QByteArray> loadedParts = d->m_persistentIndex.data(EntityTreeModel::LoadedPartsRole).value<QSet<QByteArray> >();

            Q_ASSERT(!loadedParts.contains(d->m_partName));

            Item current = d->m_persistentIndex.data(EntityTreeModel::ItemRole).value<Item>();
            current.apply(item);
            QAbstractItemModel *model = const_cast<QAbstractItemModel *>(d->m_persistentIndex.model());
            Q_ASSERT(model);
            QVariant itemVariant = QVariant::fromValue(item);
            model->setData(d->m_persistentIndex, itemVariant, EntityTreeModel::ItemRole);
            d->m_item = item;

            emitResult();
        },
        [this](const Error &/*error*/) {
            setError(KJob::UserDefinedError);
            setErrorText(i18n("Unable to fetch item for index"));
            emitResult();
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
