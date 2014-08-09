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
#include "collectioncombobox_p.h"

#include "asyncselectionhandler_p.h"
#include "collectiondialog.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/entityrightsfiltermodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/session.h>

#include <kdescendantsproxymodel.h>
#include "collectionutils_p.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QEvent>
#include <QPointer>
#include <QMouseEvent>

using namespace Akonadi;

class CollectionComboBox::Private
{
public:
    Private(QAbstractItemModel *customModel, CollectionComboBox *parent)
        : mParent(parent)
        , mMonitor(0)
        , mModel(0)
    {
        if (customModel) {
            mBaseModel = customModel;
        } else {
            mMonitor = new Akonadi::ChangeRecorder(mParent);
            mMonitor->fetchCollection(true);
            mMonitor->setCollectionMonitored(Akonadi::Collection::root());

            mModel = new EntityTreeModel(mMonitor, mParent);
            mModel->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
            mModel->setListFilter(CollectionFetchScope::Display);

            mBaseModel = mModel;
        }

        KDescendantsProxyModel *proxyModel = new KDescendantsProxyModel(parent);
        proxyModel->setDisplayAncestorData(true);
        proxyModel->setSourceModel(mBaseModel);

        mMimeTypeFilterModel = new CollectionFilterProxyModel(parent);
        mMimeTypeFilterModel->setSourceModel(proxyModel);

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

    CollectionComboBox *mParent;

    ChangeRecorder *mMonitor;
    EntityTreeModel *mModel;
    QAbstractItemModel *mBaseModel;
    CollectionFilterProxyModel *mMimeTypeFilterModel;
    EntityRightsFilterModel *mRightsFilterModel;
    AsyncSelectionHandler *mSelectionHandler;
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

MobileEventHandler::MobileEventHandler(CollectionComboBox *comboBox, CollectionFilterProxyModel *mimeTypeFilter,
                                       EntityRightsFilterModel *accessRightsFilter, QAbstractItemModel *customModel)
    : QObject(comboBox)
    , mComboBox(comboBox)
    , mMimeTypeFilter(mimeTypeFilter)
    , mAccessRightsFilter(accessRightsFilter)
    , mCustomModel(customModel)
{
}

bool MobileEventHandler::eventFilter(QObject *object, QEvent *event)
{
    if (object == mComboBox && mComboBox->isEnabled() && event->type() == QEvent::MouseButtonPress) {

        const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // we receive mouse events from other widgets as well, so check for ours
        if (mComboBox->rect().contains(mouseEvent->pos())) {
            QMetaObject::invokeMethod(this, "openDialog", Qt::QueuedConnection);
        }

        return true;
    }

    return QObject::eventFilter(object, event);
}

void MobileEventHandler::openDialog()
{
    QPointer<Akonadi::CollectionDialog> dialog(new Akonadi::CollectionDialog(mCustomModel));
    dialog->setMimeTypeFilter(mMimeTypeFilter->mimeTypeFilters());
    dialog->setAccessRightsFilter(mAccessRightsFilter->accessRights());

    if (dialog->exec() == QDialog::Accepted && dialog != 0) {
        const Akonadi::Collection collection = dialog->selectedCollection();
        const QModelIndex index = Akonadi::EntityTreeModel::modelIndexForCollection(mComboBox->model(), collection);
        mComboBox->setCurrentIndex(index.row());
    }
    delete dialog;
}

CollectionComboBox::CollectionComboBox(QWidget *parent)
    : KComboBox(parent)
    , d(new Private(0, this))
{
#ifdef KDEPIM_MOBILE_UI
    MobileEventHandler *handler = new MobileEventHandler(this, d->mMimeTypeFilterModel, d->mRightsFilterModel, d->mBaseModel);
    installEventFilter(handler);
#endif
}

CollectionComboBox::CollectionComboBox(QAbstractItemModel *model, QWidget *parent)
    : KComboBox(parent)
    , d(new Private(model, this))
{
#ifdef KDEPIM_MOBILE_UI
    MobileEventHandler *handler = new MobileEventHandler(this, d->mMimeTypeFilterModel, d->mRightsFilterModel, d->mBaseModel);
    installEventFilter(handler);
#endif
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
        foreach (const QString &mimeType, contentMimeTypes) {
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
#include "moc_collectioncombobox_p.cpp"
