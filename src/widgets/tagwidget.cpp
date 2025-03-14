/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
    SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagwidget.h"

#include "changerecorder.h"
#include "tagmodel.h"
#include "tagselectiondialog.h"

#include <KLocalizedString>

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLocale>
#include <QMenu>
#include <QToolButton>

using namespace Akonadi;

namespace Akonadi
{
class TagView : public QLineEdit
{
    Q_OBJECT
public:
    explicit TagView(QWidget *parent)
        : QLineEdit(parent)
    {
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        if (text().isEmpty()) {
            return;
        }

        QMenu menu;
        menu.addAction(i18n("Clear"), this, &TagView::clearTags);
        menu.exec(event->globalPos());
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        Q_EMIT addTags();
        QLineEdit::mousePressEvent(event);
    }

Q_SIGNALS:
    void clearTags();
    void addTags();
};

} // namespace Akonadi

// include after defining TagView
#include "ui_tagwidget.h"

class Akonadi::TagWidgetPrivate
{
public:
    Ui::TagWidget ui;
    Akonadi::Tag::List mTags;
    Akonadi::TagModel *mModel = nullptr;
};

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , d(new TagWidgetPrivate)
{
    auto monitor = new Monitor(this);
    monitor->setObjectName(QLatin1StringView("TagWidgetMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);
    d->mModel = new Akonadi::TagModel(monitor, this);
    connect(monitor, &Monitor::tagAdded, this, &TagWidget::updateView);

    d->ui.setupUi(this);
    connect(d->ui.tagView, &TagView::clearTags, this, &TagWidget::clearTags);
    connect(d->ui.tagView, &TagView::addTags, this, &TagWidget::editTags);

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
    // d->mTagView is always readOnly => not change it.
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
        const auto tag = d->mModel->data(index, Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
        if (d->mTags.contains(tag)) {
            tagsNames.push_back(tag.name());
        }
    }
    d->ui.tagView->setText(QLocale::system().createSeparatedList(tagsNames));
}

#include "tagwidget.moc"

#include "moc_tagwidget.cpp"
