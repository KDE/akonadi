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

using namespace Akonadi;

Scope::Scope()
    : mRidContext(-1)
    , mScope(Invalid)
{
}

Scope::SelectionScope Scope::scope() const
{
    return mScope;
}

bool Scope::isEmpty() const
{
    switch (mScope) {
    case Invalid:
        return true;
    case Uid:
        return mUidSet.isEmpty();
    case Rid:
        return mRidSet.isEmpty();
    case HierarchicalRid:
        return mRidChain.isEmpty();
    case Gid:
        return mGidSet.isEmpty();
    }

    Q_ASSERT(false);
    return true;
}


void Scope::setUidSet(const QVector<qint64> &uidSet)
{
    mScope = Uid;
    mUidSet = uidSet;
}

QVector<qint64> Scope::uidSet() const
{
    return mUidSet;
}

void Scope::setRidSet(const QStringList &ridSet)
{
    mScope = Rid;
    mRidSet = ridSet;
}

QStringList Scope::ridSet() const
{
    return mRidSet;
}

void Scope::setRidChain(const QStringList &ridChain)
{
    mScope = HierarchicalRid;
    mRidChain = ridChain;
}

QStringList Scope::ridChain() const
{
    return mRidChain;
}

void Scope::setRidContext(qint64 context)
{
    mRidContext = context;
}

qint64 Scope::ridContext() const
{
    return mRidContext;
}

void Scope::setGidSet(const QStringList &gidSet)
{
    mScope = Gid;
    mGidSet = gidSet;
}

QStringList Scope::gidSet() const
{
    return mGidSet;
}

qint64 Scope::uid() const
{
    if (mUidSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(mUidSet.size() == 1);
        return -1;
    }
    return mUidSet.at(0);
}

QString Scope::rid() const
{
    if (mRidSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(mRidSet.size() == 1);
        return QString();
    }
    return mRidSet.at(0);
}

QString Scope::gid() const
{
    if (mGidSet.size() != 1) {
        // TODO: Error hanlding!
        Q_ASSERT(mGidSet.size() == 1);
        return QString();
    }
    return mGidSet.at(0);
}

QDataStream &operator<<(QDataStream &stream, const Scope &scope)
{
    stream << static_cast<qint8>(scope.mScope);
    switch (scope.mScope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream << scope.mUidSet;
        return stream;
    case Scope::Rid:
        stream << scope.mRidSet;
        stream << scope.mRidContext;
        return stream;
    case Scope::HierarchicalRid:
        stream << scope.mRidChain;
        return stream;
    case Scope::Gid:
        stream << scope.mGidSet;
        return stream;
    }

    Q_ASSERT(false);
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Scope &scope)
{
    scope.mUidSet.clear();
    scope.mRidSet.clear();
    scope.mRidChain.clear();
    scope.mGidSet.clear();

    qint8 c;
    stream >> c;
    scope.mScope = static_cast<Scope::SelectionScope>(c);
    switch (scope.mScope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream >> scope.mUidSet;
        return stream;
    case Scope::Rid:
        stream >> scope.mRidSet;
        stream >> scope.mRidContext;
        return stream;
    case Scope::HierarchicalRid:
        stream >> scope.mRidChain;
        return stream;
    case Scope::Gid:
        stream >> scope.mGidSet;
        return stream;
    }

    Q_ASSERT(false);
    return stream;
}