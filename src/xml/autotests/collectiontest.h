/*
    SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef COLLECTIONTEST_H
#define COLLECTIONTEST_H

#include "collection.h"
#include <QObject>

class CollectionTest : public QObject
{
    Q_OBJECT
private:
    void verifyCollection(const Akonadi::Collection::List &colist, int listPosition, const QString &remoteId, const QString &name, const QStringList &mimeType);
private Q_SLOTS:
    void serializeCollection();
    void testBuildCollection();
};

#endif
