/*
  Copyright (c) 2015-2018 Montel Laurent <montel@kde.org>

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

#include "tagselectwidget.h"
#include "monitor.h"
#include "tagmodel.h"
#include "tageditwidget.h"

#include <QHBoxLayout>

using namespace Akonadi;

class Q_DECL_HIDDEN TagSelectWidget::Private
{
public:
    Private(TagSelectWidget *parent)
        : mParent(parent)
    {
        init();
    }
    void init();
    TagSelectWidget *mParent = nullptr;
    Akonadi::TagEditWidget *mTagEditWidget = nullptr;
};

void TagSelectWidget::Private::init()
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mParent->setLayout(mainLayout);

    Monitor *monitor = new Monitor(mParent);
    monitor->setObjectName(QStringLiteral("TagSelectWidgetMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    Akonadi::TagModel *model = new Akonadi::TagModel(monitor, mParent);
    mTagEditWidget = new Akonadi::TagEditWidget(model, mParent, true);
    mTagEditWidget->setObjectName(QStringLiteral("tageditwidget"));

    mainLayout->addWidget(mTagEditWidget);

}

TagSelectWidget::TagSelectWidget(QWidget *parent)
    : QWidget(parent),
      d(new Private(this))
{
}

TagSelectWidget::~TagSelectWidget()
{
    delete d;
}

void TagSelectWidget::setSelection(const Tag::List &tags)
{
    d->mTagEditWidget->setSelection(tags);
}

Tag::List TagSelectWidget::selection() const
{
    return d->mTagEditWidget->selection();
}

QStringList TagSelectWidget::tagToStringList() const
{
    QStringList list;
    const Akonadi::Tag::List tags = selection();
    list.reserve(tags.count());
    for (const Akonadi::Tag &tag : tags) {
        list.append(tag.url().url());
    }
    return list;
}

void TagSelectWidget::setSelectionFromStringList(const QStringList &lst)
{
    Akonadi::Tag::List tags;

    tags.reserve(lst.count());
    for (const QString &category : lst) {
        tags.append(Akonadi::Tag::fromUrl(QUrl(category)));
    }
    setSelection(tags);
}

