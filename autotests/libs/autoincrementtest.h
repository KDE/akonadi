/*
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "collection.h"

#include <QObject>

namespace Akonadi
{
class CollectionCreateJob;
class ItemCreateJob;
}

class AutoIncrementTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testItemAutoIncrement();
    void testCollectionAutoIncrement();

private:
    Akonadi::ItemCreateJob *createItemCreateJob();
    Akonadi::CollectionCreateJob *createCollectionCreateJob(int number);
    Akonadi::Collection itemTargetCollection;
    Akonadi::Collection collectionTargetCollection;
};
