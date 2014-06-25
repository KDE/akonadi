/*
    Copyright 2010 Tobias Koenig <tokoe@kde.org>

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

#include "collectiondialog_mobile_p.h"
#include "asyncselectionhandler_p.h"
#include "collectiondialog.h"

#include <qplatformdefs.h>

#include <kdescendantsproxymodel.h>

#include "changerecorder.h"
#include "collectioncreatejob.h"
#include "collectionfilterproxymodel.h"
#include "collectionutils.h"
#include "entityrightsfiltermodel.h"
#include "entitytreemodel.h"

#include <KLocalizedString>
#include <QInputDialog>
#include <QUrl>
#include <KMessageBox>
#include <KStandardDirs>

#include <QQuickView>
#include <QStandardPaths>

using namespace Akonadi;

CollectionDialog::Private::Private(QAbstractItemModel *customModel, CollectionDialog *parent, CollectionDialogOptions options)
    : QObject(parent)
    , mParent(parent)
    , mSelectionMode(QAbstractItemView::SingleSelection)
    , mOkButtonEnabled(false)
    , mCancelButtonEnabled(true)
    , mCreateButtonEnabled(false)
{
    // setup GUI
    mView = new QQuickView(mParent);
    mView->setResizeMode(QQuickView::SizeRootObjectToView);

    mParent->setMainWidget(mView);
    mParent->setButtons(KDialog::None);

    changeCollectionDialogOptions(options);

    QAbstractItemModel *baseModel;

    if (customModel) {
        baseModel = customModel;
    } else {
        mMonitor = new Akonadi::ChangeRecorder(mParent);
        mMonitor->fetchCollection(true);
        mMonitor->setCollectionMonitored(Akonadi::Collection::root());

        mModel = new EntityTreeModel(mMonitor, mParent);
        mModel->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);

        baseModel = mModel;
    }

    KDescendantsProxyModel *proxyModel = new KDescendantsProxyModel(parent);
    proxyModel->setDisplayAncestorData(true);
    proxyModel->setSourceModel(baseModel);

    mMimeTypeFilterModel = new CollectionFilterProxyModel(parent);
    mMimeTypeFilterModel->setSourceModel(proxyModel);

    mRightsFilterModel = new EntityRightsFilterModel(parent);
    mRightsFilterModel->setSourceModel(mMimeTypeFilterModel);

    mFilterModel = new QSortFilterProxyModel(parent);
    mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mFilterModel->setSourceModel(mRightsFilterModel);

    mSelectionModel = new QItemSelectionModel(mFilterModel);
    mParent->connect(mSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     SLOT(slotSelectionChanged()));
    mParent->connect(mSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     this, SLOT(selectionChanged(QItemSelection,QItemSelection)));

    mSelectionHandler = new AsyncSelectionHandler(mFilterModel, mParent);
    mParent->connect(mSelectionHandler, SIGNAL(collectionAvailable(QModelIndex)),
                     SLOT(slotCollectionAvailable(QModelIndex)));

    foreach (const QString &importPath, KGlobal::dirs()->findDirs("module", QStringLiteral("imports"))) {
        mView->engine()->addImportPath(importPath);
    }

    mView->rootContext()->setContextProperty(QStringLiteral("dialogController"), this);
    mView->rootContext()->setContextProperty(QStringLiteral("collectionModel"), mFilterModel);

    // QUICKHACK: since we have no KDE integration plugin available in kdelibs, we have to do the translation in C++ space
    mView->rootContext()->setContextProperty(QStringLiteral("okButtonText"), KStandardGuiItem::ok().text().remove(QLatin1Char('&')));
    mView->rootContext()->setContextProperty(QStringLiteral("cancelButtonText"), KStandardGuiItem::cancel().text().remove(QLatin1Char('&')));
    mView->rootContext()->setContextProperty(QStringLiteral("createButtonText"), i18n("&New Subfolder...").remove(QLatin1Char('&')));

    mView->setSource(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi-kde/qml/CollectionDialogMobile.qml"))));

#if defined (Q_WS_MAEMO_5) || defined (MEEGO_EDITION_HARMATTAN)
    mParent->setWindowState(Qt::WindowFullScreen);
#else
    // on the desktop start with a nice size
    mParent->resize(800, 480);
#endif
}

CollectionDialog::Private::~Private()
{
}

void CollectionDialog::Private::slotCollectionAvailable(const QModelIndex &index)
{
    mSelectionModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
}

void CollectionDialog::Private::slotFilterFixedString(const QString &filter)
{
}

void CollectionDialog::Private::slotSelectionChanged()
{
    mOkButtonEnabled = mSelectionModel->hasSelection();
    if (mAllowToCreateNewChildCollection) {
        const Akonadi::Collection parentCollection = mParent->selectedCollection();
        const bool canCreateChildCollections = canCreateCollection(parentCollection);
        const bool isVirtual = parentCollection.isVirtual();

        mCreateButtonEnabled = (canCreateChildCollections && !isVirtual);
        if (parentCollection.isValid()) {
            const bool canCreateItems = (parentCollection.rights() & Akonadi::Collection::CanCreateItem);
            mOkButtonEnabled = canCreateItems;
        }
    }

    emit buttonStatusChanged();
}

void CollectionDialog::Private::changeCollectionDialogOptions(CollectionDialogOptions options)
{
    mAllowToCreateNewChildCollection = (options & AllowToCreateNewChildCollection);
    emit buttonStatusChanged();
}

bool CollectionDialog::Private::canCreateCollection(const Akonadi::Collection &parentCollection) const
{
    if (!parentCollection.isValid()) {
        return false;
    }

    if ((parentCollection.rights() & Akonadi::Collection::CanCreateCollection)) {
        const QStringList dialogMimeTypeFilter = mParent->mimeTypeFilter();
        const QStringList parentCollectionMimeTypes = parentCollection.contentMimeTypes();
        Q_FOREACH (const QString &mimetype, dialogMimeTypeFilter) {
            if (parentCollectionMimeTypes.contains(mimetype)) {
                return true;
            }
        }
        return true;
    }
    return false;
}

void CollectionDialog::Private::slotAddChildCollection()
{
    const Akonadi::Collection parentCollection = mParent->selectedCollection();
    if (canCreateCollection(parentCollection)) {
        const QString name = QInputDialog::getText(mParent, i18nc("@title:window", "New Folder"),
                                                   i18nc("@label:textbox, name of a thing", "Name"));
        if (name.isEmpty()) {
            return;
        }

        Akonadi::Collection collection;
        collection.setName(name);
        collection.setParentCollection(parentCollection);
        Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob(collection);
        connect(job, SIGNAL(result(KJob*)), mParent, SLOT(slotCollectionCreationResult(KJob*)));
    }
}

void CollectionDialog::Private::slotCollectionCreationResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(mParent, i18n("Could not create folder: %1", job->errorString()),
                           i18n("Folder creation failed"));
    }
}

void CollectionDialog::Private::setDescriptionText(const QString &text)
{
    mDescriptionText = text;
    emit descriptionTextChanged();
}

QString CollectionDialog::Private::descriptionText() const
{
    return mDescriptionText;
}

bool CollectionDialog::Private::okButtonEnabled() const
{
    return mOkButtonEnabled;
}

bool CollectionDialog::Private::cancelButtonEnabled() const
{
    return mCancelButtonEnabled;
}

bool CollectionDialog::Private::createButtonEnabled() const
{
    return mCreateButtonEnabled;
}

bool CollectionDialog::Private::createButtonVisible() const
{
    return mAllowToCreateNewChildCollection;
}

void CollectionDialog::Private::okClicked()
{
    mParent->accept();
}

void CollectionDialog::Private::cancelClicked()
{
    mParent->reject();
}

void CollectionDialog::Private::createClicked()
{
    slotAddChildCollection();
}

void CollectionDialog::Private::setCurrentIndex(int row)
{
    const QModelIndex index = mSelectionModel->model()->index(row, 0);
    mSelectionModel->select(index, QItemSelectionModel::ClearAndSelect);
}

void CollectionDialog::Private::setFilterText(const QString &text)
{
    mFilterModel->setFilterFixedString(text);
}

void CollectionDialog::Private::selectionChanged(const QItemSelection &selection, const QItemSelection &)
{
    if (selection.isEmpty()) {
        return;
    }

    emit selectionChanged(selection.indexes().first().row());
}

CollectionDialog::CollectionDialog(QWidget *parent)
    : KDialog(parent, Qt::Window)
    , d(new Private(0, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(QAbstractItemModel *model, QWidget *parent)
    : KDialog(parent, Qt::Window)
    , d(new Private(model, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(CollectionDialogOptions options, QAbstractItemModel *model, QWidget *parent)
    : KDialog(parent, Qt::Window)
    , d(new Private(model, this, options))
{
}

CollectionDialog::~CollectionDialog()
{
}

Akonadi::Collection CollectionDialog::selectedCollection() const
{
    if (!d->mSelectionModel->hasSelection()) {
        return Akonadi::Collection();
    }

    return d->mSelectionModel->selectedRows().first().data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

Akonadi::Collection::List CollectionDialog::selectedCollections() const
{
    if (!d->mSelectionModel->hasSelection()) {
        return Akonadi::Collection::List();
    }

    return (Akonadi::Collection::List() << d->mSelectionModel->selectedRows().first().data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>());
}

void CollectionDialog::setMimeTypeFilter(const QStringList &mimeTypes)
{
    d->mMimeTypeFilterModel->clearFilters();
    d->mMimeTypeFilterModel->addMimeTypeFilters(mimeTypes);
}

QStringList CollectionDialog::mimeTypeFilter() const
{
    return d->mMimeTypeFilterModel->mimeTypes();
}

void CollectionDialog::setAccessRightsFilter(Collection::Rights rights)
{
    d->mRightsFilterModel->setAccessRights(rights);
}

Akonadi::Collection::Rights CollectionDialog::accessRightsFilter() const
{
    return d->mRightsFilterModel->accessRights();
}

void CollectionDialog::setDescription(const QString &text)
{
    d->setDescriptionText(text);
}

void CollectionDialog::setDefaultCollection(const Collection &collection)
{
    d->mSelectionHandler->waitForCollection(collection);
}

void CollectionDialog::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
    d->mSelectionMode = mode;
}

QAbstractItemView::SelectionMode CollectionDialog::selectionMode() const
{
    return d->mSelectionMode;
}

void CollectionDialog::changeCollectionDialogOptions(CollectionDialogOptions options)
{
    d->changeCollectionDialogOptions(options);
}

#include "moc_collectiondialog.cpp"
#include "moc_collectiondialog_mobile_p.cpp"
