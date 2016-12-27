/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "subscriptiondialog.h"

#include "controlgui.h"
#include "recursivecollectionfilterproxymodel.h"
#include "subscriptionjob_p.h"
#include "subscriptionmodel_p.h"

#include "akonadiwidgets_debug.h"
#include <ksharedconfig.h>

#include <KLocalizedString>
#include <KConfigGroup>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <krecursivefilterproxymodel.h>
#include <QHeaderView>
#include <QLabel>
#include <QTreeView>
#include <QCheckBox>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN SubscriptionDialog::Private
{
public:
    Private(SubscriptionDialog *parent)
        : q(parent)
    {
    }

    void done()
    {
        SubscriptionJob *job = new SubscriptionJob(q);
        job->subscribe(model->subscribed());
        job->unsubscribe(model->unsubscribed());
        connect(job, SIGNAL(result(KJob*)), q, SLOT(subscriptionResult(KJob*)));
    }

    void subscriptionResult(KJob *job)
    {
        if (job->error()) {
            // TODO
            qCWarning(AKONADIWIDGETS_LOG) << job->errorString();
        }
        q->deleteLater();
    }

    void modelLoaded()
    {
        collectionView->setEnabled(true);
        collectionView->expandAll();
        mOkButton->setEnabled(true);
    }

    void slotSetPattern(const QString &text)
    {
        filterRecursiveCollectionFilter->setSearchPattern(text);
        collectionView->expandAll();
    }

    void slotSetIncludeCheckedOnly(bool checked)
    {
        filterRecursiveCollectionFilter->setIncludeCheckedOnly(checked);
    }

    void writeConfig()
    {
        KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");
        group.writeEntry("Size", q->size());
    }

    void readConfig()
    {
        KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");
        const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
        if (sizeDialog.isValid()) {
            q->resize(sizeDialog);
        }
    }

    void slotUnSubscribe();
    void slotSubscribe();

    SubscriptionDialog *q;
    QTreeView *collectionView;
    QPushButton *subscribe;
    QPushButton *unSubscribe;
    SubscriptionModel *model;
    RecursiveCollectionFilterProxyModel *filterRecursiveCollectionFilter;
    QPushButton *mOkButton;

};

void SubscriptionDialog::Private::slotSubscribe()
{
    const QModelIndexList list = collectionView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
    collectionView->setFocus();
}

void SubscriptionDialog::Private::slotUnSubscribe()
{
    const QModelIndexList list = collectionView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    }
    collectionView->setFocus();
}

SubscriptionDialog::SubscriptionDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    init(QStringList());
}

SubscriptionDialog::SubscriptionDialog(const QStringList &mimetypes, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    init(mimetypes);
}

void SubscriptionDialog::showHiddenCollection(bool showHidden)
{
    d->model->showHiddenCollection(showHidden);
}

void SubscriptionDialog::init(const QStringList &mimetypes)
{
    setWindowTitle(i18n("Local Subscriptions"));
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    d->model = new SubscriptionModel(this);

    d->filterRecursiveCollectionFilter
        = new Akonadi::RecursiveCollectionFilterProxyModel(this);
    d->filterRecursiveCollectionFilter->setDynamicSortFilter(true);
    d->filterRecursiveCollectionFilter->setSourceModel(d->model);
    d->filterRecursiveCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    if (!mimetypes.isEmpty()) {
        d->filterRecursiveCollectionFilter->addContentMimeTypeInclusionFilters(mimetypes);
    }

    d->collectionView = new QTreeView(mainWidget);
    mainLayout->addWidget(d->collectionView);
    d->collectionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->collectionView->header()->hide();
    d->collectionView->setModel(d->filterRecursiveCollectionFilter);
    d->collectionView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QHBoxLayout *filterBarLayout = new QHBoxLayout;

    filterBarLayout->addWidget(new QLabel(i18n("Search:")));

    QLineEdit *lineEdit = new QLineEdit(mainWidget);
    mainLayout->addWidget(lineEdit);
    lineEdit->setClearButtonEnabled(true);
    lineEdit->setFocus();
    connect(lineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotSetPattern(QString)));
    filterBarLayout->addWidget(lineEdit);
    QCheckBox *checkBox = new QCheckBox(i18n("Subscribed only"), mainWidget);
    mainLayout->addWidget(checkBox);
    connect(checkBox, SIGNAL(clicked(bool)),
            this, SLOT(slotSetIncludeCheckedOnly(bool)));
    filterBarLayout->addWidget(checkBox);

    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->addWidget(d->collectionView);

    QVBoxLayout *subscribeButtonLayout = new QVBoxLayout;
    d->subscribe = new QPushButton(i18n("Subscribe"));
    subscribeButtonLayout->addWidget(d->subscribe);
    connect(d->subscribe, SIGNAL(clicked()), this, SLOT(slotSubscribe()));

    d->unSubscribe = new QPushButton(i18n("Unsubscribe"));
    subscribeButtonLayout->addWidget(d->unSubscribe);
    connect(d->unSubscribe, SIGNAL(clicked()), this, SLOT(slotUnSubscribe()));
    subscribeButtonLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Expanding));

    hboxLayout->addLayout(subscribeButtonLayout);

    mainLayout->addLayout(filterBarLayout);
    mainLayout->addLayout(hboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    d->mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    d->mOkButton->setDefault(true);
    d->mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);

    d->mOkButton->setEnabled(false);
    mainLayout->addWidget(buttonBox);

    connect(d->model, SIGNAL(loaded()), SLOT(modelLoaded()));
    connect(d->mOkButton, SIGNAL(clicked(bool)), this, SLOT(done()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SubscriptionDialog::deleteLater);
    ControlGui::widgetNeedsAkonadi(mainWidget);
    d->readConfig();
}

SubscriptionDialog::~SubscriptionDialog()
{
    d->writeConfig();
    delete d;
}

#include "moc_subscriptiondialog.cpp"
