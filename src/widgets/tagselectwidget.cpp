/*
  Copyright (c) 2015-2020 Laurent Montel <montel@kde.org>

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

#include "shared/akranges.h"

#include <QHBoxLayout>

using namespace Akonadi;
using namespace AkRanges;

class Q_DECL_HIDDEN TagSelectWidget::Private
{
public:
    QScopedPointer<TagEditWidget> mTagEditWidget;
};


TagSelectWidget::TagSelectWidget(QWidget *parent)
    : QWidget(parent),
      d(new Private())
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    auto *monitor = new Monitor(this);
    monitor->setObjectName(QStringLiteral("TagSelectWidgetMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    auto *model = new TagModel(monitor, this);
    d->mTagEditWidget.reset(new TagEditWidget());
    d->mTagEditWidget->setModel(model);
    d->mTagEditWidget->setSelectionEnabled(true);
    d->mTagEditWidget->setObjectName(QStringLiteral("tageditwidget"));

    mainLayout->addWidget(d->mTagEditWidget.get());
}

TagSelectWidget::~TagSelectWidget() = default;

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
    return selection() | Views::transform([](const auto &tag) { return tag.url().url(); }) | Actions::toQList;
}

void TagSelectWidget::setSelectionFromStringList(const QStringList &lst)
{
    setSelection(lst | Views::transform([](const auto &cat) { return Tag::fromUrl(QUrl{cat}); }) | Actions::toQVector);
}

