/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

namespace Akonadi
{
class Relation;

AKONADICORE_EXPORT unsigned int qHash(const Akonadi::Relation &);
}

#include <QByteArray>
#include <QDebug>
#include <QSharedDataPointer>

namespace Akonadi
{
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
    using List = QVector<Relation>;

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

    Relation(const Relation &);
    Relation(Relation &&) noexcept;
    ~Relation();

    Relation &operator=(const Relation &);
    Relation &operator=(Relation &&) noexcept;

    bool operator==(const Relation &) const;
    bool operator!=(const Relation &) const;

    /**
     * Sets the @p item of the left side of the relation.
     */
    void setLeft(const Item &item);

    /**
     * Returns the identifier of the left side of the relation.
     */
    Q_REQUIRED_RESULT Item left() const;

    /**
     * Sets the @p item of the right side of the relation.
     */
    void setRight(const Akonadi::Item &item);

    /**
     * Returns the identifier of the right side of the relation.
     */
    Q_REQUIRED_RESULT Item right() const;

    /**
     * Sets the type of the relation.
     */
    void setType(const QByteArray &type);

    /**
     * Returns the type of the relation.
     */
    Q_REQUIRED_RESULT QByteArray type() const;

    /**
     * Sets the remote id of the relation.
     */
    void setRemoteId(const QByteArray &type);

    /**
     * Returns the remote id of the relation.
     */
    Q_REQUIRED_RESULT QByteArray remoteId() const;

    Q_REQUIRED_RESULT bool isValid() const;

private:
    struct Private;
    QSharedDataPointer<Private> d;
};

}

AKONADICORE_EXPORT QDebug &operator<<(QDebug &debug, const Akonadi::Relation &tag);

Q_DECLARE_METATYPE(Akonadi::Relation)
Q_DECLARE_METATYPE(Akonadi::Relation::List)
Q_DECLARE_METATYPE(QSet<Akonadi::Relation>)
Q_DECLARE_TYPEINFO(Akonadi::Relation, Q_MOVABLE_TYPE);
