/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "categorieseditwidget.h"

#include "tagwidget.h"

#include <akonadi/item.h>
#include <akonadi/tag.h>
#include <kabc/addressee.h>
#include <kdialog.h>

#include <QHBoxLayout>

CategoriesEditWidget::CategoriesEditWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(KDialog::spacingHint());

    mTagWidget = new TagWidget(this);
    layout->addWidget(mTagWidget);
}

CategoriesEditWidget::~CategoriesEditWidget()
{
}

void CategoriesEditWidget::setReadOnly(bool readOnly)
{
    mTagWidget->setEnabled(!readOnly);
}

void CategoriesEditWidget::loadContact(const KABC::Addressee &contact)
{
    Akonadi::Tag::List tags;

    const QStringList categories = contact.categories();
    foreach (const QString &category, categories) {
        tags.append(Akonadi::Tag::fromUrl(category));
    }

    mTagWidget->setTags(tags);
}

void CategoriesEditWidget::storeContact(KABC::Addressee &contact) const
{
    QStringList categories;

    const Akonadi::Tag::List tags = mTagWidget->tags();
    foreach (const Akonadi::Tag &tag, tags) {
        categories.append(tag.url().url());
    }

    contact.setCategories(categories);
}
