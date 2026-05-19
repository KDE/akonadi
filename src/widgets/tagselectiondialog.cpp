/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagselectiondialog.h"
#include "controlgui.h"
#include "monitor.h"
#include "tagmodel.h"
#include "ui_tagselectiondialog.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QPushButton>

using namespace Akonadi;
namespace
{
static const char myTagSelectionDialogGroupName[] = "TagSelectionDialog";
}
class Akonadi::TagSelectionDialogPrivate
{
public:
    explicit TagSelectionDialogPrivate(QDialog *parent)
        : q(parent)
    {
    }

    void writeConfig() const;
    void readConfig() const;

    QDialog *const q;
    Ui::TagSelectionDialog ui;
    QStringList mInitialTagNames;
    QPushButton *mOkButton = nullptr;
};

void TagSelectionDialogPrivate::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myTagSelectionDialogGroupName));
    group.writeEntry("Size", q->size());
}

void TagSelectionDialogPrivate::readConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myTagSelectionDialogGroupName));
    const QSize sizeDialog = group.readEntry("Size", QSize(500, 400));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

void TagSelectionDialog::init()
{
    d->mOkButton = buttons()->button(QDialogButtonBox::Ok);
    d->mOkButton->setEnabled(false);

    connect(d->ui.tagWidget->listView(), &QAbstractItemView::clicked, this, [this]([[maybe_unused]] const QModelIndex &index) {
        QStringList selectedTagNames;
        for (const Akonadi::Tag &tag : selection()) {
            selectedTagNames << tag.name();
        }
        selectedTagNames.sort();
        d->mOkButton->setEnabled(d->mInitialTagNames != selectedTagNames);
    });
}

TagSelectionDialog::TagSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , d(new TagSelectionDialogPrivate(this))
{
    d->ui.setupUi(this);

    auto monitor = new Monitor(this);
    monitor->setObjectName(QLatin1StringView("TagSelectionDialogMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    d->ui.tagWidget->setModel(new TagModel(monitor, this));
    d->ui.tagWidget->setSelectionEnabled(true);

    d->readConfig();

    ControlGui::widgetNeedsAkonadi(this);

    init();
}

TagSelectionDialog::TagSelectionDialog(TagModel *model, QWidget *parent)
    : QDialog(parent)
    , d(new TagSelectionDialogPrivate(this))
{
    d->ui.setupUi(this);

    d->ui.tagWidget->setModel(model);
    d->ui.tagWidget->setSelectionEnabled(true);

    d->readConfig();

    ControlGui::widgetNeedsAkonadi(this);

    init();
}

TagSelectionDialog::~TagSelectionDialog()
{
    d->writeConfig();
}

QDialogButtonBox *TagSelectionDialog::buttons() const
{
    return d->ui.buttonBox;
}

Tag::List TagSelectionDialog::selection() const
{
    return d->ui.tagWidget->selection();
}

void TagSelectionDialog::setSelection(const Tag::List &tags)
{
    if (d->mInitialTagNames.isEmpty()) {
        for (const Akonadi::Tag &tag : tags) {
            d->mInitialTagNames << tag.name();
        }
    }
    d->mInitialTagNames.sort();

    d->ui.tagWidget->setSelection(tags);
}

#include "moc_tagselectiondialog.cpp"
