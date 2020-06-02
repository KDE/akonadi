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
#include "monitor.h"

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
#include <KMessageBox>
#include <qnamespace.h>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN SubscriptionDialog::Private
{
public:
    explicit Private(SubscriptionDialog *parent)
        : q(parent)
        , model(&monitor, parent)
    {
        ui.setupUi(q);

        connect(&model, &SubscriptionModel::modelLoaded, q, [this]() {
            filterRecursiveCollectionFilter.sort(0, Qt::AscendingOrder);
            ui.collectionView->setEnabled(true);
            ui.collectionView->expandAll();
            ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        });

        filterRecursiveCollectionFilter.setSourceModel(&model);
        filterRecursiveCollectionFilter.setFilterCaseSensitivity(Qt::CaseInsensitive);
        filterRecursiveCollectionFilter.setSortRole(Qt::DisplayRole);
        filterRecursiveCollectionFilter.setSortCaseSensitivity(Qt::CaseSensitive);
        filterRecursiveCollectionFilter.setSortLocaleAware(true);

        ui.collectionView->setModel(&filterRecursiveCollectionFilter);
        ui.searchLineEdit->setFocus();
        q->connect(ui.searchLineEdit, &QLineEdit::textChanged,
                   q, [this](const QString &str) {
                        filterRecursiveCollectionFilter.setSearchPattern(str);
                        ui.collectionView->expandAll();
                   });
        q->connect(ui.subscribedOnlyCheckBox, &QCheckBox::toggled,
                   q, [this](bool state) {
                        filterRecursiveCollectionFilter.setIncludeCheckedOnly(state);
                   });
        q->connect(ui.subscribeButton, &QPushButton::clicked,
                   q, [this]() { toggleSubscribed(Qt::Checked); });
        q->connect(ui.unsubscribeButton, &QPushButton::clicked,
                   q, [this]() { toggleSubscribed(Qt::Unchecked); });

        auto *okButton = ui.buttonBox->button(QDialogButtonBox::Ok);
        okButton->setEnabled(false);
        connect(okButton, &QPushButton::clicked, q, [this] () { done(); });
    }

    void done()
    {
        SubscriptionJob *job = new SubscriptionJob(q);
        job->subscribe(model.subscribed());
        job->unsubscribe(model.unsubscribed());
        connect(job, &SubscriptionJob::result, q, [this](KJob *job) {
            if (job->error()) {
                qCWarning(AKONADIWIDGETS_LOG) << job->errorString();
                KMessageBox::sorry(q, i18n("Failed to update subscription: %1", job->errorString()),
                                   i18nc("@title", "Subscription Error"));
                q->reject();
            }
            q->accept();
        });
    }

    void writeConfig() const
    {
        KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");
        group.writeEntry("Size", q->size());
    }

    void readConfig() const
    {
        KConfigGroup group(KSharedConfig::openConfig(), "SubscriptionDialog");
        const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
        if (sizeDialog.isValid()) {
            q->resize(sizeDialog);
        }
    }

    void toggleSubscribed(Qt::CheckState state)
    {
        const QModelIndexList list = ui.collectionView->selectionModel()->selectedIndexes();
        for (const QModelIndex &index : list) {
            model.setData(index, state, Qt::CheckStateRole);
        }
        ui.collectionView->setFocus();
    }


    SubscriptionDialog * const q;
    Ui::SubscriptionDialog ui;

    Monitor monitor;
    SubscriptionModel model;
    RecursiveCollectionFilterProxyModel filterRecursiveCollectionFilter;

};


SubscriptionDialog::SubscriptionDialog(QWidget *parent)
    : SubscriptionDialog({}, parent)
{
}

SubscriptionDialog::SubscriptionDialog(const QStringList &mimetypes, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    setAttribute(Qt::WA_DeleteOnClose);

    if (!mimetypes.isEmpty()) {
        d->filterRecursiveCollectionFilter.addContentMimeTypeInclusionFilters(mimetypes);
    }
    ControlGui::widgetNeedsAkonadi(this);
    d->readConfig();
}

SubscriptionDialog::~SubscriptionDialog()
{
    d->writeConfig();
}

void SubscriptionDialog::showHiddenCollection(bool showHidden)
{
    d->model.setShowHiddenCollections(showHidden);
}


#include "moc_subscriptiondialog.cpp"
