/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "tagselectwidget.h"
#include "monitor.h"
#include "tagmodel.h"
#include "tageditwidget.h"

#include <QHBoxLayout>

using namespace Akonadi;

class TagSelectWidget::Private
{
public:
    Private(TagSelectWidget *parent)
        : mParent(parent)
    {
        init();
    }
    void init();
    TagSelectWidget *mParent;
    Akonadi::TagEditWidget *mTagEditWidget;
};

void TagSelectWidget::Private::init()
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mParent->setLayout(mainLayout);

    Monitor *monitor = new Monitor(mParent);
    monitor->setTypeMonitored(Monitor::Tags);

    Akonadi::TagModel *model = new Akonadi::TagModel(monitor, mParent);
    mTagEditWidget = new Akonadi::TagEditWidget(model, mParent, true);
    mTagEditWidget->setObjectName(QLatin1String("tageditwidget"));

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

