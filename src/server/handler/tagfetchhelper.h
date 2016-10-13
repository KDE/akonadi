/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_TAGFETCHHELPER_H
#define AKONADI_TAGFETCHHELPER_H

#include <QObject>
#include <QSqlQuery>

#include <private/scope_p.h>

namespace Akonadi {

class ImapSet;

namespace Server {

class Connection;
class Response;

class TagFetchHelper : public QObject
{
    Q_OBJECT

public:
    TagFetchHelper(Connection *connection, const Scope &scope);

    bool fetchTags();

    static QMap<QByteArray, QByteArray> fetchTagAttributes(qint64 tagId);

private:
    QSqlQuery buildTagQuery();
    QSqlQuery buildAttributeQuery() const;
    static QSqlQuery buildAttributeQuery(qint64 id);

private:
    Connection *mConnection;
    Scope mScope;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_TAGFETCHHELPER_H
