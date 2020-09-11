/*
    This file is part of Akonadi Contact.

    SPDX-FileCopyrightText: 2007-2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectioncombobox.h"

#include "asyncselectionhandler_p.h"
#include "collectiondialog.h"

#include "monitor.h"
#include "collectionfetchscope.h"
#include "collectionfilterproxymodel.h"
#include "entityrightsfiltermodel.h"
#include "entitytreemodel.h"
#include "session.h"
#include "collectionutils.h"

#include <KDescendantsProxyModel>

#include <QAbstractItemModel>

using namespace Akonadi;

class Q_DECL_HIDDEN CollectionComboBox::Private
{
public:
    Private(QAbstractItemModel *customModel, CollectionComboBox *parent)
        : mParent(parent)
    {
        if (customModel) {
            mBaseModel = customModel;
        } else {
            mMonitor = new Akonadi::Monitor(mParent);
            mMonitor->setObjectName(QStringLiteral("CollectionComboBoxMonitor"));
            mMonitor->fetchCollection(true);
            mMonitor->setCollectionMonitored(Akonadi::Collection::root());

            // This ETM will be set to only show collections with the wanted mimetype in setMimeTypeFilter
            mModel = new EntityTreeModel(mMonitor, mParent);
            mModel->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
            mModel->setListFilter(CollectionFetchScope::Display);

            mBaseModel = mModel;
        }

        // Flatten the tree, e.g.
        // Kolab
        // Kolab / Inbox
        // Kolab / Inbox / Calendar
        KDescendantsProxyModel *proxyModel = new KDescendantsProxyModel(parent);
        proxyModel->setDisplayAncestorData(true);
        proxyModel->setSourceModel(mBaseModel);

        // Filter it by mimetype again, to only keep
        // Kolab / Inbox / Calendar
        mMimeTypeFilterModel = new CollectionFilterProxyModel(parent);
        mMimeTypeFilterModel->setSourceModel(proxyModel);

        // Filter by access rights. TODO: maybe this functionality could be provided by CollectionFilterProxyModel, to save one proxy?
        mRightsFilterModel = new EntityRightsFilterModel(parent);
        mRightsFilterModel->setSourceModel(mMimeTypeFilterModel);

        mParent->setModel(mRightsFilterModel);
        mParent->model()->sort(mParent->modelColumn());

        mSelectionHandler = new AsyncSelectionHandler(mRightsFilterModel, mParent);
        mParent->connect(mSelectionHandler, &AsyncSelectionHandler::collectionAvailable,
                         mParent, [this](const auto &mi) { activated(mi); });
    }

    ~Private() = default;

    void activated(int index);
    void activated(const QModelIndex &index);

    CollectionComboBox *const mParent;

    Monitor *mMonitor = nullptr;
    EntityTreeModel *mModel = nullptr;
    QAbstractItemModel *mBaseModel = nullptr;
    CollectionFilterProxyModel *mMimeTypeFilterModel = nullptr;
    EntityRightsFilterModel *mRightsFilterModel = nullptr;
    AsyncSelectionHandler *mSelectionHandler = nullptr;
};

void CollectionComboBox::Private::activated(int index) 
{
    const QModelIndex modelIndex = mParent->model()->index(index, 0);
    if (modelIndex.isValid()) {
        Q_EMIT mParent->currentChanged(modelIndex.data(EntityTreeModel::CollectionRole).value<Collection>());
    }
}

void CollectionComboBox::Private::activated(const QModelIndex &index)
{
    mParent->setCurrentIndex(index.row());
}

CollectionComboBox::CollectionComboBox(QWidget *parent)
    : QComboBox(parent)
    , d(new Private(nullptr, this))
{
}

CollectionComboBox::CollectionComboBox(QAbstractItemModel *model, QWidget *parent)
    : QComboBox(parent)
    , d(new Private(model, this))
{
}

CollectionComboBox::~CollectionComboBox()
{
    delete d;
}

void CollectionComboBox::setMimeTypeFilter(const QStringList &contentMimeTypes)
{
    d->mMimeTypeFilterModel->clearFilters();
    d->mMimeTypeFilterModel->addMimeTypeFilters(contentMimeTypes);

    if (d->mMonitor) {
        for (const QString &mimeType : contentMimeTypes) {
            d->mMonitor->setMimeTypeMonitored(mimeType, true);
        }
    }
}

QStringList CollectionComboBox::mimeTypeFilter() const
{
    return d->mMimeTypeFilterModel->mimeTypeFilters();
}

void CollectionComboBox::setAccessRightsFilter(Collection::Rights rights)
{
    d->mRightsFilterModel->setAccessRights(rights);
}

Akonadi::Collection::Rights CollectionComboBox::accessRightsFilter() const
{
    return d->mRightsFilterModel->accessRights();
}

void CollectionComboBox::setDefaultCollection(const Collection &collection)
{
    d->mSelectionHandler->waitForCollection(collection);
}

Akonadi::Collection CollectionComboBox::currentCollection() const
{
    const QModelIndex modelIndex = model()->index(currentIndex(), 0);
    if (modelIndex.isValid()) {
        return modelIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Collection>();
    } else {
        return Akonadi::Collection();
    }
}

void CollectionComboBox::setExcludeVirtualCollections(bool b)
{
    d->mMimeTypeFilterModel->setExcludeVirtualCollections(b);
}

bool CollectionComboBox::excludeVirtualCollections() const
{
    return d->mMimeTypeFilterModel->excludeVirtualCollections();
}

#include "moc_collectioncombobox.cpp"
