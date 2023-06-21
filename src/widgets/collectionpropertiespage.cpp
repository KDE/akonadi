/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionpropertiespage.h"

using namespace Akonadi;

/// @cond PRIVATE

/**
 * @internal
 */
class Akonadi::CollectionPropertiesPagePrivate
{
public:
    QString title;
};

/// @endcond

CollectionPropertiesPage::CollectionPropertiesPage(QWidget *parent)
    : QWidget(parent)
    , d(new CollectionPropertiesPagePrivate)
{
}

CollectionPropertiesPage::~CollectionPropertiesPage() = default;

bool CollectionPropertiesPage::canHandle(const Collection &collection) const
{
    Q_UNUSED(collection)
    return true;
}

QString Akonadi::CollectionPropertiesPage::pageTitle() const
{
    return d->title;
}

void CollectionPropertiesPage::setPageTitle(const QString &title)
{
    d->title = title;
}

CollectionPropertiesPageFactory::~CollectionPropertiesPageFactory()
{
}

#include "moc_collectionpropertiespage.cpp"
