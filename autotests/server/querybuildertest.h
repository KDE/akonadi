/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_QUERYBUILDERTEST_H
#define AKONADI_QUERYBUILDERTEST_H

#undef QT_NO_CAST_FROM_ASCII

#include <QObject>

namespace Akonadi
{
namespace Server
{
class QueryBuilder;
}
}

class QueryBuilderTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testQueryBuilder_data();
    void testQueryBuilder();
    void benchQueryBuilder();

private:
    QList<Akonadi::Server::QueryBuilder> mBuilders;
};

#endif
