/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSqlQuery>

#include <private/protocol_p.h>
#include <private/scope_p.h>

namespace Akonadi
{
namespace Server
{
class Connection;

class TagFetchHelper
{
public:
    TagFetchHelper(Connection *connection, const Scope &scope, const Protocol::TagFetchScope &fetchScope);
    ~TagFetchHelper() = default;

    bool fetchTags();

    static QMap<QByteArray, QByteArray> fetchTagAttributes(qint64 tagId, const Protocol::TagFetchScope &fetchScope);

private:
    QSqlQuery buildTagQuery();
    QSqlQuery buildAttributeQuery() const;
    static QSqlQuery buildAttributeQuery(qint64 id, const Protocol::TagFetchScope &fetchScope);

private:
    Connection *mConnection = nullptr;
    Scope mScope;
    Protocol::TagFetchScope mFetchScope;
};

} // namespace Server
} // namespace Akonadi
