/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
    SPDX-FileCopyrightText: 2016-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagwidget.h"

#include "tagmodel.h"
#include "changerecorder.h"
#include "tagselectiondialog.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QToolButton>
#include <QMenu>
#include <QContextMenuEvent>
#include <QLocale>

using namespace Akonadi;

namespace Akonadi
{

class Q_DECL_HIDDEN TagView : public QLineEdit
{
    Q_OBJECT
public:
    explicit TagView(QWidget *parent)
        : QLineEdit(parent)
    {}

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        if (text().isEmpty()) {
            return;
        }

        QMenu menu;
        menu.addAction(i18n("Clear"), this, &TagView::clearTags);
        menu.exec(event->globalPos());
    }

Q_SIGNALS:
    void clearTags();
};

} // namespace Akonadi

// include after defining TagView
#include "ui_tagwidget.h"

class Q_DECL_HIDDEN TagWidget::Private
{
public:
    Ui::TagWidget ui;
    Akonadi::Tag::List mTags;
    Akonadi::TagModel *mModel = nullptr;
};

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    auto *monitor = new Monitor(this);
    monitor->setObjectName(QStringLiteral("TagWidgetMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);
    d->mModel = new Akonadi::TagModel(monitor, this);
    connect(monitor, &Monitor::tagAdded, this, &TagWidget::updateView);

    d->ui.setupUi(this);
    connect(d->ui.tagView, &TagView::clearTags, this, &TagWidget::clearTags);

    connect(d->ui.editButton, &QToolButton::clicked, this, &TagWidget::editTags);
    connect(d->mModel, &Akonadi::TagModel::populated, this, &TagWidget::updateView);
}

TagWidget::~TagWidget() = default;

void TagWidget::clearTags()
{
    if (!d->mTags.isEmpty()) {
        d->mTags.clear();
        d->ui.tagView->clear();
        Q_EMIT selectionChanged(d->mTags);
    }
}

void TagWidget::setSelection(const Akonadi::Tag::List &tags)
{
    if (d->mTags != tags) {
        d->mTags = tags;
        updateView();
        Q_EMIT selectionChanged(d->mTags);
    }
}

Akonadi::Tag::List TagWidget::selection() const
{
    return d->mTags;
}

void TagWidget::setReadOnly(bool readOnly)
{
    d->ui.editButton->setEnabled(!readOnly);
    //d->mTagView is always readOnly => not change it.
}

void TagWidget::editTags()
{
    QScopedPointer<Akonadi::TagSelectionDialog> dlg(new TagSelectionDialog(d->mModel, this));
    dlg->setSelection(d->mTags);
    if (dlg->exec() == QDialog::Accepted) {
        d->mTags = dlg->selection();
        updateView();
        Q_EMIT selectionChanged(d->mTags);
    }
}

void TagWidget::updateView()
{
    QStringList tagsNames;
    // Load the real tag names from the model
    for (int i = 0; i < d->mModel->rowCount(); ++i) {
        const QModelIndex index = d->mModel->index(i, 0);
        const Akonadi::Tag tag = d->mModel->data(index, Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
        if (d->mTags.contains(tag)) {
            tagsNames.push_back(tag.name());
        }
    }
    d->ui.tagView->setText(QLocale::system().createSeparatedList(tagsNames));
}

#include "tagwidget.moc"

