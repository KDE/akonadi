/*
 *  Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *  Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef AKONADI_PRIVATE_SCOPE_P_H
#define AKONADI_PRIVATE_SCOPE_P_H

#include "akonadiprivate_export.h"

#include <QtCore/QSharedDataPointer>

class QDataStream;
class QString;
class QStringList;

namespace Akonadi
{
class ImapSet;
class ImapInterval;
class Scope;
}

AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Scope &scope);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Scope &scope);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug debug, const Akonadi::Scope &scope);

namespace Akonadi
{

class ScopePrivate;
class AKONADIPRIVATE_EXPORT Scope
{
public:
    enum SelectionScope : uchar {
        Invalid = 0,
        Uid = 1 << 0,
        Rid = 1 << 1,
        HierarchicalRid = 1 << 2,
        Gid = 1 << 3
    };

    class HRID {
    public:
        explicit HRID();
        HRID(qint64 id, const QString &remoteId = QString());
        HRID(const HRID &other);
        HRID(HRID &&other);

        HRID &operator=(const HRID &other);
        HRID &operator=(HRID &&other);

        bool isEmpty() const;
        bool operator==(const HRID &other) const;

        qint64 id;
        QString remoteId;
    };

    explicit Scope();
    Scope(SelectionScope scope, const QStringList &ids);

    /* UID */
    Scope(qint64 id);
    Scope(const ImapSet &uidSet);
    Scope(const ImapInterval &interval);
    Scope(const QVector<qint64> &interval);
    Scope(const QVector<HRID> &hridChain);

    Scope(const Scope &other);
    Scope(Scope &&other);
    ~Scope();

    Scope &operator=(const Scope &other);
    Scope &operator=(Scope &&other);

    bool operator==(const Scope &other) const;
    bool operator!=(const Scope &other) const;

    SelectionScope scope() const;

    bool isEmpty() const;

    ImapSet uidSet() const;
    void setUidSet(const ImapSet &uidSet);

    void setRidSet(const QStringList &ridSet);
    QStringList ridSet() const;

    void setHRidChain(const QVector<HRID> &ridChain);
    QVector<HRID> hridChain() const;

    void setGidSet(const QStringList &gidChain);
    QStringList gidSet() const;

    qint64 uid() const;
    QString rid() const;
    QString gid() const;

private:
    QSharedDataPointer<ScopePrivate> d;
    friend class ScopePrivate;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Scope &scope);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Scope &scope);
};

} // namespace Akonadi

#endif
