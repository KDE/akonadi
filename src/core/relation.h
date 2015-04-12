/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_RELATION_H
#define AKONADI_RELATION_H

#include "akonadicore_export.h"

namespace Akonadi {
class Relation;
}

AKONADICORE_EXPORT unsigned int qHash(const Akonadi::Relation &);

#include <QSharedPointer>
#include <QByteArray>
#include <QList>
#include <QDebug>

namespace Akonadi {
class Item;

/**
 * An Akonadi Relation.
 *
 * A Relation object represents an relation between two Akonadi items.
 *
 * An example usecase could be a association of a note with an email. The note (that for instance contains personal notes for the email),
 * can be stored independently but is easily retrieved by asking for relations the email.
 * 
 * The relation type allows to distinguish various types of relations that could for instance be bidirectional or not.
 * 
 * @since 4.15
 */
class AKONADICORE_EXPORT Relation
{
public:
    typedef QList<Relation> List;

    /**
     * The GENERIC type represents a generic relation between two items.
     */
    static const char *GENERIC;

    /**
     * Creates an invalid relation.
     */
    Relation();

    /**
     * Creates a relation
     */
    explicit Relation(const QByteArray &type, const Item &left, const Item &right);

    Relation(const Relation &other);
    ~Relation();

    Relation &operator=(const Relation &);
    bool operator==(const Relation &) const;
    bool operator!=(const Relation &) const;

    /**
     * Sets the @p item of the left side of the relation.
     */
    void setLeft(const Item &item);

    /**
     * Returns the identifier of the left side of the relation.
     */
    Item left() const;

    /**
     * Sets the @p item of the right side of the relation.
     */
    void setRight(const Akonadi::Item &item);

    /**
     * Returns the identifier of the right side of the relation.
     */
    Item right() const;

    /**
     * Sets the type of the relation.
     */
    void setType(const QByteArray &type) const;

    /**
     * Returns the type of the relation.
     */
    QByteArray type() const;

    /**
     * Sets the remote id of the relation.
     */
    void setRemoteId(const QByteArray &type) const;

    /**
     * Returns the remote idof the relation.
     */
    QByteArray remoteId() const;

    bool isValid() const;

private:
    class Private;
    QSharedPointer<Private> d;
};

}

AKONADICORE_EXPORT QDebug &operator<<(QDebug &debug, const Akonadi::Relation &tag);

Q_DECLARE_METATYPE(Akonadi::Relation)
Q_DECLARE_METATYPE(Akonadi::Relation::List)
Q_DECLARE_METATYPE(QSet<Akonadi::Relation>)

#endif
