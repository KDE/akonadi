/*
    This file is part of Akonadi

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
    Copyright (C) 2016-2017 Laurent Montel <montel@kde.org>

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

#include "tagwidget.h"

#include "tagmodel.h"
#include "changerecorder.h"
#include "tagselectiondialog.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QContextMenuEvent>
using namespace Akonadi;

class Q_DECL_HIDDEN TagWidget::Private
{
public:
    Private()
        : mTagView(nullptr),
          mModel(nullptr)
    {

    }

    TagView *mTagView;
    Akonadi::Tag::List mTags;
    Akonadi::TagModel *mModel;
};

TagView::TagView(QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText(i18n("Click to Add Tags"));
    setReadOnly(true);
}

void TagView::contextMenuEvent(QContextMenuEvent *event)
{
    if (text().isEmpty()) {
        return;
    }

    QMenu menu;
    menu.addAction(i18n("Clear"), this, SIGNAL(clearTags()));

    menu.exec(event->globalPos());
}

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    Monitor *monitor = new Monitor(this);
    monitor->setTypeMonitored(Monitor::Tags);
    d->mModel = new Akonadi::TagModel(monitor, this);
    connect(monitor, &Monitor::tagAdded, this, &TagWidget::updateView);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    d->mTagView = new TagView(this);
    connect(d->mTagView, &TagView::clearTags, this, &TagWidget::clearTags);
    layout->addWidget(d->mTagView);

    QToolButton *editButton = new QToolButton(this);
    editButton->setText(i18n("..."));
    layout->addWidget(editButton, Qt::AlignRight);

    layout->setStretch(0, 10);

    connect(editButton, &QToolButton::clicked, this, &TagWidget::editTags);
    connect(d->mModel, &Akonadi::TagModel::populated, this, &TagWidget::updateView);
}

TagWidget::~TagWidget()
{
}

void TagWidget::clearTags()
{
    if (!d->mTags.isEmpty()) {
        d->mTags.clear();
        d->mTagView->clear();
        emit selectionChanged(d->mTags);
    }
}

void TagWidget::setSelection(const Akonadi::Tag::List &tags)
{
    if (d->mTags != tags) {
        d->mTags = tags;
        updateView();
    }
}

Akonadi::Tag::List TagWidget::selection() const
{
    return d->mTags;
}

void TagWidget::editTags()
{
    QScopedPointer<Akonadi::TagSelectionDialog> dlg(new TagSelectionDialog(this));
    dlg->setSelection(d->mTags);
    if (dlg->exec() == QDialog::Accepted) {
        d->mTags = dlg->selection();
        updateView();
        emit selectionChanged(d->mTags);
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
            tagsNames << tag.name();
        }
    }
    d->mTagView->setText(tagsNames.join(QStringLiteral(", ")));
}
