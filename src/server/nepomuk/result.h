/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef _NEPOMUK_SEARCH_RESULT_H_
#define _NEPOMUK_SEARCH_RESULT_H_

#include <QtCore/QSharedDataPointer>
#include <QtCore/QUrl>
#include <QtCore/QList>
#include <QtCore/QHash>

#include "soprano/statement.h"

namespace Nepomuk {
namespace Query {
/**
 * \brief A single search result.
 *
 * A search returns a set of Result object.
 *
 * \author Sebastian Trueg <trueg@kde.org>
 */
class Result
{
public:
    Result();
    explicit Result(const QUrl &uri, double score = 0.0);
    Result(const Result &other);
    ~Result();

    Result &operator=(const Result &other);

    double score() const;
    QUrl resourceUri() const;

    void setScore(double score);

    void addRequestProperty(const QUrl &property, const Soprano::Node &value);

    QHash<QUrl, Soprano::Node> requestProperties() const;

    Soprano::Node operator[](const QUrl &property) const;
    Soprano::Node requestProperty(const QUrl &property) const;

    bool operator==(const Result &other) const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};
}
}

Q_DECLARE_TYPEINFO(Nepomuk::Query::Result, Q_MOVABLE_TYPE);

#endif
