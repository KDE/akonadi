/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

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

#include <kdescendantsproxymodel.h>

#include <QAbstractItemModel>

using namespace Akonadi;

class Q_DECL_HIDDEN CollectionComboBox::Private
{
public:
    Private(QAbstractItemModel *customModel, CollectionComboBox *parent)
        : mParent(parent)
        , mMonitor(nullptr)
        , mModel(nullptr)
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
        mParent->connect(mSelectionHandler, SIGNAL(collectionAvailable(QModelIndex)),
                         mParent, SLOT(activated(QModelIndex)));

        mParent->connect(mParent, SIGNAL(activated(int)),
                         mParent, SLOT(activated(int)));
    }

    ~Private()
    {
    }

    void activated(int index);
    void activated(const QModelIndex &index);

    CollectionComboBox *mParent = nullptr;

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
        emit mParent->currentChanged(modelIndex.data(EntityTreeModel::CollectionRole).value<Collection>());
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
