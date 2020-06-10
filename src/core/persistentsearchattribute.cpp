/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "persistentsearchattribute.h"
#include "collection.h"


#include "private/imapparser_p.h"

#include <QString>
#include <QStringList>

using namespace Akonadi;

class Q_DECL_HIDDEN PersistentSearchAttribute::Private
{
public:
    Private()
        : remote(false)
        , recursive(false)
    {
    }

    QString queryString;
    QVector<qint64> queryCollections;
    bool remote;
    bool recursive;
};

PersistentSearchAttribute::PersistentSearchAttribute()
    : d(new Private)
{
}

PersistentSearchAttribute::~PersistentSearchAttribute()
{
    delete d;
}

QString PersistentSearchAttribute::queryString() const
{
    return d->queryString;
}

void PersistentSearchAttribute::setQueryString(const QString &query)
{
    d->queryString = query;
}

QVector<qint64> PersistentSearchAttribute::queryCollections() const
{
    return d->queryCollections;
}

void PersistentSearchAttribute::setQueryCollections(const QVector<Collection> &collections)
{
    d->queryCollections.clear();
    d->queryCollections.reserve(collections.count());
    for (const Collection &collection : collections) {
        d->queryCollections << collection.id();
    }
}

void PersistentSearchAttribute::setQueryCollections(const QVector<qint64> &collectionsIds)
{
    d->queryCollections = collectionsIds;
}

bool PersistentSearchAttribute::isRecursive() const
{
    return d->recursive;
}

void PersistentSearchAttribute::setRecursive(bool recursive)
{
    d->recursive = recursive;
}

bool PersistentSearchAttribute::isRemoteSearchEnabled() const
{
    return d->remote;
}

void PersistentSearchAttribute::setRemoteSearchEnabled(bool enabled)
{
    d->remote = enabled;
}

QByteArray PersistentSearchAttribute::type() const
{
    static const QByteArray sType("PERSISTENTSEARCH");
    return sType;
}

Attribute *PersistentSearchAttribute::clone() const
{
    PersistentSearchAttribute *attr = new PersistentSearchAttribute;
    attr->setQueryString(queryString());
    attr->setQueryCollections(queryCollections());
    attr->setRecursive(isRecursive());
    attr->setRemoteSearchEnabled(isRemoteSearchEnabled());
    return attr;
}

QByteArray PersistentSearchAttribute::serialized() const
{
    QStringList cols;
    cols.reserve(d->queryCollections.count());
    for (qint64 colId : qAsConst(d->queryCollections)) {
        cols << QString::number(colId);
    }

    QList<QByteArray> l;
    // ### eventually replace with the AKONADI_PARAM_PERSISTENTSEARCH_XXX constants
    l.append("QUERYSTRING");
    l.append(ImapParser::quote(d->queryString.toUtf8()));
    l.append("QUERYCOLLECTIONS");
    l.append("(" + cols.join(QLatin1Char(' ')).toLatin1() + ')');
    if (d->remote) {
        l.append("REMOTE");
    }
    if (d->recursive) {
        l.append("RECURSIVE");
    }
    return "(" + ImapParser::join(l, " ") + ')';   //krazy:exclude=doublequote_chars
}

void PersistentSearchAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    const int listSize(l.size());
    for (int i = 0; i < listSize; ++i) {
        const QByteArray &key = l.at(i);
        if (key == QByteArrayLiteral("QUERYLANGUAGE")) {
            // Skip the value
            ++i;
        } else if (key == QByteArrayLiteral("QUERYSTRING")) {
            d->queryString = QString::fromUtf8(l.at(i + 1));
            ++i;
        } else if (key == QByteArrayLiteral("QUERYCOLLECTIONS")) {
            QList<QByteArray> ids;
            ImapParser::parseParenthesizedList(l.at(i + 1), ids);
            d->queryCollections.clear();
            d->queryCollections.reserve(ids.count());
            for (const QByteArray &id : qAsConst(ids)) {
                d->queryCollections << id.toLongLong();
            }
            ++i;
        } else if (key == QByteArrayLiteral("REMOTE")) {
            d->remote = true;
        } else if (key == QByteArrayLiteral("RECURSIVE")) {
            d->recursive = true;
        }
    }
}
