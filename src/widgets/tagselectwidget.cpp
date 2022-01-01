/*
  SPDX-FileCopyrightText: 2015-2022 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
  */

#include "tagselectwidget.h"
#include "monitor.h"
#include "tageditwidget.h"
#include "tagmodel.h"

#include "shared/akranges.h"

#include <QHBoxLayout>

using namespace Akonadi;
using namespace AkRanges;

class Akonadi::TagSelectWidgetPrivate
{
public:
    QScopedPointer<TagEditWidget> mTagEditWidget;
};

TagSelectWidget::TagSelectWidget(QWidget *parent)
    : QWidget(parent)
    , d(new TagSelectWidgetPrivate())
{
    auto mainLayout = new QHBoxLayout(this);

    auto monitor = new Monitor(this);
    monitor->setObjectName(QStringLiteral("TagSelectWidgetMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    auto model = new TagModel(monitor, this);
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
    return selection() | Views::transform([](const auto &tag) {
               return tag.url().url();
           })
        | Actions::toQList;
}

void TagSelectWidget::setSelectionFromStringList(const QStringList &lst)
{
    setSelection(lst | Views::transform([](const auto &cat) {
                     return Tag::fromUrl(QUrl{cat});
                 })
                 | Actions::toQVector);
}
