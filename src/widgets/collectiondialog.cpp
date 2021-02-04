/*
    SPDX-FileCopyrightText: 2008 Ingo Klöcker <kloecker@kde.org>
    SPDX-FileCopyrightText: 2010-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiondialog.h"

#include "asyncselectionhandler_p.h"

#include "collectioncreatejob.h"
#include "collectionfetchscope.h"
#include "collectionfilterproxymodel.h"
#include "collectionutils.h"
#include "entityrightsfiltermodel.h"
#include "entitytreemodel.h"
#include "entitytreeview.h"
#include "monitor.h"
#include "session.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

using namespace Akonadi;

class Q_DECL_HIDDEN CollectionDialog::Private
{
public:
    Private(QAbstractItemModel *customModel, CollectionDialog *parent, CollectionDialogOptions options)
        : mParent(parent)
    {
        // setup GUI
        auto layout = new QVBoxLayout(mParent);

        mTextLabel = new QLabel(mParent);
        layout->addWidget(mTextLabel);
        mTextLabel->hide();

        auto filterCollectionLineEdit = new QLineEdit(mParent);
        filterCollectionLineEdit->setClearButtonEnabled(true);
        filterCollectionLineEdit->setPlaceholderText(
            i18nc("@info Displayed grayed-out inside the "
                  "textbox, verb to search",
                  "Search"));
        layout->addWidget(filterCollectionLineEdit);

        mView = new EntityTreeView(mParent);
        mView->setDragDropMode(QAbstractItemView::NoDragDrop);
        mView->header()->hide();
        layout->addWidget(mView);

        mUseByDefault = new QCheckBox(i18n("Use folder by default"), mParent);
        mUseByDefault->hide();
        layout->addWidget(mUseByDefault);

        mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, mParent);
        mParent->connect(mButtonBox, &QDialogButtonBox::accepted, mParent, &QDialog::accept);
        mParent->connect(mButtonBox, &QDialogButtonBox::rejected, mParent, &QDialog::reject);
        layout->addWidget(mButtonBox);
        mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        // setup models
        QAbstractItemModel *baseModel = nullptr;

        if (customModel) {
            baseModel = customModel;
        } else {
            mMonitor = new Akonadi::Monitor(mParent);
            mMonitor->setObjectName(QStringLiteral("CollectionDialogMonitor"));
            mMonitor->fetchCollection(true);
            mMonitor->setCollectionMonitored(Akonadi::Collection::root());

            auto model = new EntityTreeModel(mMonitor, mParent);
            model->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
            model->setListFilter(CollectionFetchScope::Display);
            baseModel = model;
        }

        mMimeTypeFilterModel = new CollectionFilterProxyModel(mParent);
        mMimeTypeFilterModel->setSourceModel(baseModel);
        mMimeTypeFilterModel->setExcludeVirtualCollections(true);

        mRightsFilterModel = new EntityRightsFilterModel(mParent);
        mRightsFilterModel->setSourceModel(mMimeTypeFilterModel);

        mFilterCollection = new QSortFilterProxyModel(mParent);
        mFilterCollection->setRecursiveFilteringEnabled(true);
        mFilterCollection->setSourceModel(mRightsFilterModel);
        mFilterCollection->setFilterCaseSensitivity(Qt::CaseInsensitive);
        mView->setModel(mFilterCollection);

        changeCollectionDialogOptions(options);
        mParent->connect(filterCollectionLineEdit, &QLineEdit::textChanged, mParent, [this](const QString &str) {
            slotFilterFixedString(str);
        });

        mParent->connect(mView->selectionModel(), &QItemSelectionModel::selectionChanged, mParent, [this]() {
            slotSelectionChanged();
        });
        mParent->connect(mView, qOverload<const QModelIndex &>(&QAbstractItemView::doubleClicked), mParent, [this]() {
            slotDoubleClicked();
        });

        mSelectionHandler = new AsyncSelectionHandler(mFilterCollection, mParent);
        mParent->connect(mSelectionHandler, &AsyncSelectionHandler::collectionAvailable, mParent, [this](const QModelIndex &index) {
            slotCollectionAvailable(index);
        });
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
        KConfig config(QStringLiteral("akonadi_contactrc"));
        KConfigGroup group(&config, QStringLiteral("CollectionDialog"));
        const QSize size = group.readEntry("Size", QSize(800, 500));
        if (size.isValid()) {
            mParent->resize(size);
        }
    }

    void writeConfig() const
    {
        KConfig config(QStringLiteral("akonadi_contactrc"));
        KConfigGroup group(&config, QStringLiteral("CollectionDialog"));
        group.writeEntry("Size", mParent->size());
        group.sync();
    }

    CollectionDialog *const mParent;

    Monitor *mMonitor = nullptr;
    CollectionFilterProxyModel *mMimeTypeFilterModel = nullptr;
    EntityRightsFilterModel *mRightsFilterModel = nullptr;
    EntityTreeView *mView = nullptr;
    AsyncSelectionHandler *mSelectionHandler = nullptr;
    QLabel *mTextLabel = nullptr;
    QSortFilterProxyModel *mFilterCollection = nullptr;
    QCheckBox *mUseByDefault = nullptr;
    QStringList mContentMimeTypes;
    QDialogButtonBox *mButtonBox = nullptr;
    QPushButton *mNewSubfolderButton = nullptr;
    bool mAllowToCreateNewChildCollection = false;
    bool mKeepTreeExpanded = false;

    void slotDoubleClicked();
    void slotSelectionChanged();
    void slotAddChildCollection();
    void slotCollectionCreationResult(KJob *job);
    bool canCreateCollection(const Akonadi::Collection &parentCollection) const;
    void changeCollectionDialogOptions(CollectionDialogOptions options);
    bool canSelectCollection() const;
};

void CollectionDialog::Private::slotDoubleClicked()
{
    if (canSelectCollection()) {
        mParent->accept();
    }
}

bool CollectionDialog::Private::canSelectCollection() const
{
    bool result = (!mView->selectionModel()->selectedIndexes().isEmpty());
    if (mAllowToCreateNewChildCollection) {
        const Akonadi::Collection parentCollection = mParent->selectedCollection();

        if (parentCollection.isValid()) {
            result = (parentCollection.rights() & Akonadi::Collection::CanCreateItem);
        }
    }
    return result;
}

void CollectionDialog::Private::slotSelectionChanged()
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mView->selectionModel()->selectedIndexes().isEmpty());
    if (mAllowToCreateNewChildCollection) {
        const Akonadi::Collection parentCollection = mParent->selectedCollection();
        const bool canCreateChildCollections = canCreateCollection(parentCollection);

        mNewSubfolderButton->setEnabled(canCreateChildCollections && !parentCollection.isVirtual());
        if (parentCollection.isValid()) {
            const bool canCreateItems = (parentCollection.rights() & Akonadi::Collection::CanCreateItem);
            mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(canCreateItems);
        }
    }
}

void CollectionDialog::Private::changeCollectionDialogOptions(CollectionDialogOptions options)
{
    mAllowToCreateNewChildCollection = (options & AllowToCreateNewChildCollection);
    if (mAllowToCreateNewChildCollection) {
        mNewSubfolderButton = mButtonBox->addButton(i18n("&New Subfolder..."), QDialogButtonBox::NoRole);
        mNewSubfolderButton->setIcon(QIcon::fromTheme(QStringLiteral("folder-new")));
        mNewSubfolderButton->setToolTip(i18n("Create a new subfolder under the currently selected folder"));
        mNewSubfolderButton->setEnabled(false);
        connect(mNewSubfolderButton, &QPushButton::clicked, mParent, [this]() {
            slotAddChildCollection();
        });
    }
    mKeepTreeExpanded = (options & KeepTreeExpanded);
    if (mKeepTreeExpanded) {
        mParent->connect(mRightsFilterModel, &EntityRightsFilterModel::rowsInserted, mView, &EntityTreeView::expandAll, Qt::UniqueConnection);
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
        for (const QString &mimetype : dialogMimeTypeFilter) {
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
        const QString name = QInputDialog::getText(mParent, i18nc("@title:window", "New Folder"), i18nc("@label:textbox, name of a thing", "Name"));
        if (name.trimmed().isEmpty()) {
            return;
        }

        Akonadi::Collection collection;
        collection.setName(name);
        collection.setParentCollection(parentCollection);
        if (!mContentMimeTypes.isEmpty()) {
            collection.setContentMimeTypes(mContentMimeTypes);
        }
        auto job = new Akonadi::CollectionCreateJob(collection);
        connect(job, &Akonadi::CollectionCreateJob::result, mParent, [this](KJob *job) {
            slotCollectionCreationResult(job);
        });
    }
}

void CollectionDialog::Private::slotCollectionCreationResult(KJob *job)
{
    if (job->error()) {
        QMessageBox::critical(mParent, i18n("Folder creation failed"), i18n("Could not create folder: %1", job->errorString()));
    }
}

CollectionDialog::CollectionDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(nullptr, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(QAbstractItemModel *model, QWidget *parent)
    : QDialog(parent)
    , d(new Private(model, this, CollectionDialog::None))
{
}

CollectionDialog::CollectionDialog(CollectionDialogOptions options, QAbstractItemModel *model, QWidget *parent)
    : QDialog(parent)
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
    for (const QModelIndex &index : selectedIndexes) {
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
        for (const QString &mimetype : mimeTypes) {
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

void CollectionDialog::setContentMimeTypes(const QStringList &mimetypes)
{
    d->mContentMimeTypes = mimetypes;
}

#include "moc_collectiondialog.cpp"
