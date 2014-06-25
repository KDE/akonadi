/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
    Copyright 2010 Laurent Montel <montel@kde.org>

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

#include "collectiondialog.h"

#include "asyncselectionhandler_p.h"

#include "changerecorder.h"
#include "collectionfetchscope.h"
#include "collectionfilterproxymodel.h"
#include "entityrightsfiltermodel.h"
#include "entitytreemodel.h"
#include "entitytreeview.h"
#include "session.h"
#include "collectioncreatejob.h"
#include "collectionutils.h"

#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QCheckBox>

#include <QLineEdit>
#include <KLocalizedString>
#include <QInputDialog>
#include <KMessageBox>
#include <KConfig>

using namespace Akonadi;

class CollectionDialog::Private
{
public:
    Private(QAbstractItemModel *customModel, CollectionDialog *parent, CollectionDialogOptions options)
        : mParent(parent)
        , mMonitor(0)
    {
        // setup GUI
        QWidget *widget = mParent->mainWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);

        mTextLabel = new QLabel;
        layout->addWidget(mTextLabel);
        mTextLabel->hide();

        QLineEdit *filterCollectionLineEdit = new QLineEdit(widget);
        filterCollectionLineEdit->setClearButtonEnabled(true);
        filterCollectionLineEdit->setPlaceholderText(i18nc("@info/plain Displayed grayed-out inside the "
                                                        "textbox, verb to search", "Search"));
        layout->addWidget(filterCollectionLineEdit);

        mView = new EntityTreeView;
        mView->setDragDropMode(QAbstractItemView::NoDragDrop);
        mView->header()->hide();
        layout->addWidget(mView);

        mUseByDefault = new QCheckBox(i18n("Use folder by default"));
        mUseByDefault->hide();
        layout->addWidget(mUseByDefault);

        mParent->enableButton(KDialog::Ok, false);

        // setup models
        QAbstractItemModel *baseModel;

        if (customModel) {
            baseModel = customModel;
        } else {
            mMonitor = new Akonadi::ChangeRecorder(mParent);
            mMonitor->fetchCollection(true);
            mMonitor->setCollectionMonitored(Akonadi::Collection::root());

            EntityTreeModel *model = new EntityTreeModel(mMonitor, mParent);
            model->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
            baseModel = model;
        }

        mMimeTypeFilterModel = new CollectionFilterProxyModel(mParent);
        mMimeTypeFilterModel->setSourceModel(baseModel);
        mMimeTypeFilterModel->setExcludeVirtualCollections(true);

        mRightsFilterModel = new EntityRightsFilterModel(mParent);
        mRightsFilterModel->setSourceModel(mMimeTypeFilterModel);

        mFilterCollection = new KRecursiveFilterProxyModel(mParent);
        mFilterCollection->setDynamicSortFilter(true);
        mFilterCollection->setSourceModel(mRightsFilterModel);
        mFilterCollection->setFilterCaseSensitivity(Qt::CaseInsensitive);
        mView->setModel(mFilterCollection);

        changeCollectionDialogOptions(options);
        mParent->connect(filterCollectionLineEdit, SIGNAL(textChanged(QString)),
                         mParent, SLOT(slotFilterFixedString(QString)));

        mParent->connect(mView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                         mParent, SLOT(slotSelectionChanged()));

        mParent->connect(mView, SIGNAL(doubleClicked(QModelIndex)),
                         mParent, SLOT(accept()));

        mSelectionHandler = new AsyncSelectionHandler(mFilterCollection, mParent);
        mParent->connect(mSelectionHandler, SIGNAL(collectionAvailable(QModelIndex)),
                         mParent, SLOT(slotCollectionAvailable(QModelIndex)));
        readConfig();
    }

    ~Private()
    {
        writeConfig();
    }

    void slotCollectionAvailable(const QModelIndex &index)
    {
        mView->expandAll();
        mView->setCurrentIndex(index);
    }

    void slotFilterFixedString(const QString &filter)
    {
        mFilterCollection->setFilterFixedString(filter);
        if (mKeepTreeExpanded) {
            mView->expandAll();
        }
    }

    void readConfig()
    {
        KConfig config( QLatin1String( "akonadi_contactrc" ) );
        KConfigGroup group( &config, QLatin1String( "CollectionDialog" ) );
        const QSize size = group.readEntry( "Size", QSize(800,  500) );
        if ( size.isValid() ) {
            mParent->resize( size );
        }
    }

    void writeConfig()
    {
        KConfig config( QLatin1String( "akonadi_contactrc" ) );
        KConfigGroup group( &config, QLatin1String( "CollectionDialog" ) );
        group.writeEntry( "Size", mParent->size() );
        group.sync();
    }

    CollectionDialog *mParent;

    ChangeRecorder *mMonitor;
    CollectionFilterProxyModel *mMimeTypeFilterModel;
    EntityRightsFilterModel *mRightsFilterModel;
    EntityTreeView *mView;
    AsyncSelectionHandler *mSelectionHandler;
    QLabel *mTextLabel;
    bool mAllowToCreateNewChildCollection;
    bool mKeepTreeExpanded;
    KRecursiveFilterProxyModel *mFilterCollection;
    QCheckBox *mUseByDefault;

    void slotSelectionChanged();
    void slotAddChildCollection();
    void slotCollectionCreationResult(KJob *job);
    bool canCreateCollection(const Akonadi::Collection &parentCollection) const;
    void changeCollectionDialogOptions(CollectionDialogOptions options);

};

void CollectionDialog::Private::slotSelectionChanged()
{
    mParent->enableButton(KDialog::Ok, mView->selectionModel()->selectedIndexes().count() > 0);
    if (mAllowToCreateNewChildCollection) {
        const Akonadi::Collection parentCollection = mParent->selectedCollection();
        const bool canCreateChildCollections = canCreateCollection(parentCollection);

        mParent->enableButton(KDialog::User1, (canCreateChildCollections && !parentCollection.isVirtual()));
        if (parentCollection.isValid()) {
            const bool canCreateItems = (parentCollection.rights() & Akonadi::Collection::CanCreateItem);
            mParent->enableButton(KDialog::Ok, canCreateItems);
        }
    }
}

void CollectionDialog::Private::changeCollectionDialogOptions(CollectionDialogOptions options)
{
    mAllowToCreateNewChildCollection = (options & AllowToCreateNewChildCollection);
    if (mAllowToCreateNewChildCollection) {
        mParent->setButtons(Ok | Cancel | User1);
        mParent->setButtonGuiItem(User1, KGuiItem(i18n("&New Subfolder..."), QStringLiteral("folder-new"),
                                                  i18n("Create a new subfolder under the currently selected folder")));
        mParent->enableButton(KDialog::User1, false);
        connect(mParent, SIGNAL(user1Clicked()), mParent, SLOT(slotAddChildCollection()));
    }
    mKeepTreeExpanded = (options & KeepTreeExpanded);
    if (mKeepTreeExpanded) {
        mParent->connect(mRightsFilterModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                         mView, SLOT(expandAll()), Qt::UniqueConnection);
        mView->expandAll();
    }
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

CollectionDialog::CollectionDialog(QWidget *parent)
    : KDialog(parent)
    , d(new Private(0, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(QAbstractItemModel *model, QWidget *parent)
    : KDialog(parent)
    , d(new Private(model, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(CollectionDialogOptions options, QAbstractItemModel *model, QWidget *parent)
    : KDialog(parent)
    , d(new Private(model, this, options))
{
}

CollectionDialog::~CollectionDialog()
{
    delete d;
}

Akonadi::Collection CollectionDialog::selectedCollection() const
{
    if (selectionMode() == QAbstractItemView::SingleSelection) {
        const QModelIndex index = d->mView->currentIndex();
        if (index.isValid()) {
            return index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
        }
    }

    return Collection();
}

Akonadi::Collection::List CollectionDialog::selectedCollections() const
{
    Collection::List collections;
    const QItemSelectionModel *selectionModel = d->mView->selectionModel();
    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    foreach (const QModelIndex &index, selectedIndexes) {
        if (index.isValid()) {
            const Collection collection = index.model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
            if (collection.isValid()) {
                collections.append(collection);
            }
        }
    }

    return collections;
}

void CollectionDialog::setMimeTypeFilter(const QStringList &mimeTypes)
{
    if (mimeTypeFilter() == mimeTypes) {
        return;
    }

    d->mMimeTypeFilterModel->clearFilters();
    d->mMimeTypeFilterModel->addMimeTypeFilters(mimeTypes);

    if (d->mMonitor) {
        foreach (const QString &mimetype, mimeTypes) {
            d->mMonitor->setMimeTypeMonitored(mimetype);
        }
    }
}

QStringList CollectionDialog::mimeTypeFilter() const
{
    return d->mMimeTypeFilterModel->mimeTypeFilters();
}

void CollectionDialog::setAccessRightsFilter(Collection::Rights rights)
{
    if (accessRightsFilter() == rights) {
        return;
    }
    d->mRightsFilterModel->setAccessRights(rights);
}

Akonadi::Collection::Rights CollectionDialog::accessRightsFilter() const
{
    return d->mRightsFilterModel->accessRights();
}

void CollectionDialog::setDescription(const QString &text)
{
    d->mTextLabel->setText(text);
    d->mTextLabel->show();
}

void CollectionDialog::setDefaultCollection(const Collection &collection)
{
    d->mSelectionHandler->waitForCollection(collection);
}

void CollectionDialog::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
    d->mView->setSelectionMode(mode);
}

QAbstractItemView::SelectionMode CollectionDialog::selectionMode() const
{
    return d->mView->selectionMode();
}

void CollectionDialog::changeCollectionDialogOptions(CollectionDialogOptions options)
{
    d->changeCollectionDialogOptions(options);
}

void CollectionDialog::setUseFolderByDefault(bool b)
{
    d->mUseByDefault->setChecked(b);
    d->mUseByDefault->show();
}

bool CollectionDialog::useFolderByDefault() const
{
    return d->mUseByDefault->isChecked();
}

#include "moc_collectiondialog.cpp"
