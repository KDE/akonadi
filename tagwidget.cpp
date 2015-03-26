/*
    This file is part of Akonadi

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include <kicon.h>
#include <klocalizedstring.h>
#include <ksqueezedtextlabel.h>

#include <QHBoxLayout>
#include <QToolButton>

using namespace Akonadi;

struct TagWidget::Private {
    QLabel *mTagLabel;
    Akonadi::Tag::List mTags;
    Akonadi::TagModel *mModel;
};

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    Monitor *monitor = new Monitor(this);
    monitor->setTypeMonitored(Monitor::Tags);
    d->mModel = new Akonadi::TagModel(monitor, this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    d->mTagLabel = new KSqueezedTextLabel;
    d->mTagLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    layout->addWidget(d->mTagLabel);

    QToolButton *editButton = new QToolButton;
    editButton->setText(i18n("..."));
    layout->addWidget(editButton, Qt::AlignRight);

    layout->setStretch(0, 10);

    connect(editButton, SIGNAL(clicked()), SLOT(editTags()));
}

TagWidget::~TagWidget()
{
}

void TagWidget::setSelection(const Akonadi::Tag::List &tags)
{
    d->mTags = tags;
    updateView();
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
    d->mTagLabel->setText(tagsNames.join(QLatin1String(", ")));
}
