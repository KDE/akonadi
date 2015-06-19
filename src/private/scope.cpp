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

#include "scope_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QStringList>

#include "imapset_p.h"

using namespace Akonadi;


namespace Akonadi
{

class ScopePrivate : public QSharedData
{
public:
    ScopePrivate()
        : ridContext(-1)
        , scope(Scope::Invalid)
    {}

    ImapSet uidSet;
    QStringList ridSet;
    QVector<Scope::HRID> hridChain;
    QStringList gidSet;
    qint64 ridContext;
    Scope::SelectionScope scope;
};

}

Scope::HRID::HRID()
    : id(-1)
{}

Scope::HRID::HRID(qint64 id, const QString &remoteId)
    : id(id)
    , remoteId(remoteId)
{}

Scope::HRID::HRID(const HRID &other)
    : id(other.id)
    , remoteId(other.remoteId)
{}

Scope::HRID::HRID(HRID &&other)
    : id(other.id)
{
    remoteId.swap(other.remoteId);
}

Scope::HRID &Scope::HRID::operator=(const HRID &other)
{
    if (*this == other) {
        return *this;
    }

    id = other.id;
    remoteId = other.remoteId;
    return *this;
}

Scope::HRID &Scope::HRID::operator=(HRID &&other)
{
    if (*this == other) {
        return *this;
    }

    id = other.id;
    remoteId.swap(other.remoteId);
    return *this;
}

bool Scope::HRID::isEmpty() const
{
    return id < 0 && remoteId.isEmpty();
}

bool Scope::HRID::operator==(const HRID &other) const
{
    return id == other.id && remoteId == other.remoteId;
}



Scope::Scope()
    : d(new ScopePrivate)
{
}

Scope::Scope(qint64 id)
    : d(new ScopePrivate)
{
    setUidSet(id);
}

Scope::Scope(const ImapSet &set)
    : d(new ScopePrivate)
{
    setUidSet(set);
}

Scope::Scope(const ImapInterval &interval)
    : d(new ScopePrivate)
{
    setUidSet(interval);
}

Scope::Scope(const QVector<qint64> &interval)
    : d(new ScopePrivate)
{
    setUidSet(interval);
}

Scope::Scope(SelectionScope scope, const QStringList &ids)
    : d(new ScopePrivate)
{
    Q_ASSERT(scope == Rid || scope == Gid);
    if (scope == Rid) {
        d->scope = scope;
        d->ridSet = ids;
    } else if (scope == Gid) {
        d->scope = scope;
        d->gidSet = ids;
    }
}

Scope::Scope(const QVector<HRID> &hrid)
    : d(new ScopePrivate)
{
    d->scope = HierarchicalRid;
    d->hridChain = hrid;
}

Scope::Scope(const Scope &other)
    : d(other.d)
{
}

Scope::Scope(Scope &&other)
{
    d.swap(other.d);
}

Scope::~Scope()
{
}

Scope &Scope::operator=(const Scope &other)
{
    d = other.d;
    return *this;
}

Scope &Scope::operator=(Scope &&other)
{
    d.swap(other.d);
    return *this;
}

bool Scope::operator==(const Scope &other) const
{
    if (d->scope != other.d->scope) {
        return false;
    }

    switch (d->scope) {
    case Uid:
        return d->uidSet == other.d->uidSet;
    case Gid:
        return d->gidSet == other.d->gidSet;
    case Rid:
        return d->ridSet == other.d->ridSet && d->ridContext == other.d->ridContext;
    case HierarchicalRid:
        return d->hridChain == other.d->hridChain && d->ridContext == other.d->ridContext;
    case Invalid:
        return true;
    }

    return false;
}

bool Scope::operator!=(const Scope &other) const
{
    return !(*this == other);
}

Scope::SelectionScope Scope::scope() const
{
    return d->scope;
}

bool Scope::isEmpty() const
{
    switch (d->scope) {
    case Invalid:
        return true;
    case Uid:
        return d->uidSet.isEmpty();
    case Rid:
        return d->ridSet.isEmpty();
    case HierarchicalRid:
        return d->hridChain.isEmpty();
    case Gid:
        return d->gidSet.isEmpty();
    }

    Q_ASSERT(false);
    return true;
}


void Scope::setUidSet(const ImapSet &uidSet)
{
    d->scope = Uid;
    d->uidSet = uidSet;
}

ImapSet Scope::uidSet() const
{
    return d->uidSet;
}

void Scope::setRidSet(const QStringList &ridSet)
{
    d->scope = Rid;
    d->ridSet = ridSet;
}

QStringList Scope::ridSet() const
{
    return d->ridSet;
}

void Scope::setHRidChain(const QVector<HRID> &hridChain)
{
    d->scope = HierarchicalRid;
    d->hridChain = hridChain;
}

QVector<Scope::HRID> Scope::hridChain() const
{
    return d->hridChain;
}

void Scope::setRidContext(qint64 context)
{
    d->ridContext = context;
}

qint64 Scope::ridContext() const
{
    return d->ridContext;
}

void Scope::setGidSet(const QStringList &gidSet)
{
    d->scope = Gid;
    d->gidSet = gidSet;
}

QStringList Scope::gidSet() const
{
    return d->gidSet;
}

qint64 Scope::uid() const
{
    if (d->uidSet.intervals().size() == 1 &&
        d->uidSet.intervals().at(0).size() == 1) {
        return d->uidSet.intervals().at(0).begin();
    }

    // TODO: Error handling!
    return -1;
}

QString Scope::rid() const
{
    if (d->ridSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(d->ridSet.size() == 1);
        return QString();
    }
    return d->ridSet.at(0);
}

QString Scope::gid() const
{
    if (d->gidSet.size() != 1) {
        // TODO: Error hanlding!
        Q_ASSERT(d->gidSet.size() == 1);
        return QString();
    }
    return d->gidSet.at(0);
}

QDataStream &operator<<(QDataStream &stream, const Akonadi::Scope &scope)
{
    stream << (quint8) scope.d->scope;
    switch (scope.d->scope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream << scope.d->uidSet;
        return stream;
    case Scope::Rid:
        stream << scope.d->ridSet;
        stream << scope.d->ridContext;
        return stream;
    case Scope::HierarchicalRid:
        stream << scope.d->hridChain;
        return stream;
    case Scope::Gid:
        stream << scope.d->gidSet;
        return stream;
    }

    Q_ASSERT(false);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Akonadi::Scope::HRID &hrid)
{
    return stream << hrid.id << hrid.remoteId;
}

QDataStream &operator>>(QDataStream &stream, Akonadi::Scope::HRID &hrid)
{
    return stream >> hrid.id >> hrid.remoteId;
}

QDataStream &operator>>(QDataStream &stream, Akonadi::Scope &scope)
{
    scope.d->uidSet = ImapSet();
    scope.d->ridSet.clear();
    scope.d->hridChain.clear();
    scope.d->gidSet.clear();

    stream >> reinterpret_cast<quint8&>(scope.d->scope);
    switch (scope.d->scope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream >> scope.d->uidSet;
        return stream;
    case Scope::Rid:
        stream >> scope.d->ridSet;
        stream >> scope.d->ridContext;
        return stream;
    case Scope::HierarchicalRid:
        stream >> scope.d->hridChain;
        return stream;
    case Scope::Gid:
        stream >> scope.d->gidSet;
        return stream;
    }

    Q_ASSERT(false);
    return stream;
}

QDebug operator<<(QDebug dbg, const Akonadi::Scope::HRID &hrid)
{
    return dbg.nospace() << "(ID: " << hrid.id << ", RemoteID: " << hrid.remoteId << ")";
}

QDebug operator<<(QDebug dbg, const Akonadi::Scope &scope)
{
    switch (scope.scope()) {
    case Scope::Uid:
        return dbg.nospace() << "UID " << scope.uidSet();
    case Scope::Rid:
        dbg.nospace() << "RID ";
        if (scope.ridContext() > -1) {
            dbg.nospace() << "(context = " << scope.ridContext() << ") ";
        }
        return dbg << scope.ridSet();
    case Scope::Gid:
        return dbg.nospace() << "GID " << scope.gidSet();
    case Scope::HierarchicalRid:
        return dbg.nospace() << "HRID " << scope.hridChain();
    default:
        return dbg.nospace() << "Invalid scope";
    }
}
