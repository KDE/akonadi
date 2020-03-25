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
#include "ui_subscriptiondialog.h"

#include "controlgui.h"
#include "recursivecollectionfilterproxymodel.h"
#include "subscriptionjob_p.h"
#include "subscriptionmodel_p.h"

#include "akonadiwidgets_debug.h"
#include <KSharedConfig>

#include <KLocalizedString>
#include <KConfigGroup>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
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
        connect(job, &SubscriptionJob::result, q, [this](KJob *job) { subscriptionResult(job); });
    }

    void subscriptionResult(KJob *job)
    {
        if (job->error()) {
            // TODO
            qCWarning(AKONADIWIDGETS_LOG) << job->errorString();
        }
        q->accept();
    }

    void modelLoaded()
    {
        filterRecursiveCollectionFilter->sort(0, Qt::AscendingOrder);
        ui.collectionView->setEnabled(true);
        ui.collectionView->expandAll();
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }

    void slotSetPattern(const QString &text)
    {
        filterRecursiveCollectionFilter->setSearchPattern(text);
        ui.collectionView->expandAll();
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

    SubscriptionDialog *q = nullptr;
    Ui::SubscriptionDialog ui;
    SubscriptionModel *model = nullptr;
    RecursiveCollectionFilterProxyModel *filterRecursiveCollectionFilter = nullptr;

};

void SubscriptionDialog::Private::slotSubscribe()
{
    const QModelIndexList list = ui.collectionView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        model->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
    ui.collectionView->setFocus();
}

void SubscriptionDialog::Private::slotUnSubscribe()
{
    const QModelIndexList list = ui.collectionView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    }
    ui.collectionView->setFocus();
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
    setAttribute(Qt::WA_DeleteOnClose);

    d->ui.setupUi(this);

    d->model = new SubscriptionModel(this);
    d->filterRecursiveCollectionFilter = new RecursiveCollectionFilterProxyModel(this);
    d->filterRecursiveCollectionFilter->setSourceModel(d->model);
    d->filterRecursiveCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->filterRecursiveCollectionFilter->setSortRole(Qt::DisplayRole);
    d->filterRecursiveCollectionFilter->setSortCaseSensitivity(Qt::CaseSensitive);
    d->filterRecursiveCollectionFilter->setSortLocaleAware(true);
    if (!mimetypes.isEmpty()) {
        d->filterRecursiveCollectionFilter->addContentMimeTypeInclusionFilters(mimetypes);
    }

    d->ui.collectionView->setModel(d->filterRecursiveCollectionFilter);
    d->ui.searchLineEdit->setFocus();
    connect(d->ui.searchLineEdit, &QLineEdit::textChanged, this, [this](const QString &str) { d->slotSetPattern(str); });
    connect(d->ui.subscribedOnlyCheckBox, &QCheckBox::toggled, this, [this](bool state) { d->slotSetIncludeCheckedOnly(state); });
    connect(d->ui.subscribeButton, &QPushButton::clicked, this, [this]() { d->slotSubscribe(); });
    connect(d->ui.unsubscribeButton, &QPushButton::clicked, this, [this]() { d->slotUnSubscribe(); });

    auto okButton = d->ui.buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    connect(d->model, &SubscriptionModel::loaded, this, [this]() { d->modelLoaded(); });
    connect(okButton, &QPushButton::clicked, this, [this] () { d->done(); });

    ControlGui::widgetNeedsAkonadi(this);
    d->readConfig();
}

SubscriptionDialog::~SubscriptionDialog()
{
    d->writeConfig();
}

#include "moc_subscriptiondialog.cpp"
