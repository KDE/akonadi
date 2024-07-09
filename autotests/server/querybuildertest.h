/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    std::vector<Akonadi::Server::QueryBuilder> mBuilders;
};
