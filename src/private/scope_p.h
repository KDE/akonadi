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

class QDataStream;

#include <QStringList>
#include <QVector>

#include "imapset_p.h"

namespace Akonadi
{
class Scope;
}
AKONADIPRIVATE_EXPORT QDataStream &operator<<(QDataStream &stream, const Akonadi::Scope &scope);
AKONADIPRIVATE_EXPORT QDataStream &operator>>(QDataStream &stream, Akonadi::Scope &scope);


namespace Akonadi
{

class AKONADIPRIVATE_EXPORT Scope
{
public:
    enum SelectionScope : char {
        Invalid = 0,
        Uid = 1 << 0,
        Rid = 1 << 1,
        HierarchicalRid = 1 << 2,
        Gid = 1 << 3
    };

    Scope();

    SelectionScope scope() const;

    bool isEmpty() const;

    ImapSet uidSet() const;
    void setUidSet(const ImapSet &uidSet);

    void setRidContext(qint64 context);
    qint64 ridContext() const;

    void setRidSet(const QStringList &ridSet);
    QStringList ridSet() const;

    void setRidChain(const QStringList &ridChain);
    QStringList ridChain() const;

    void setGidSet(const QStringList &gidChain);
    QStringList gidSet() const;

    qint64 uid() const;
    QString rid() const;
    QString gid() const;
private:
    ImapSet mUidSet;
    QStringList mRidSet;
    QStringList mRidChain;
    QStringList mGidSet;
    qint64 mRidContext;
    SelectionScope mScope;

    friend QDataStream &::operator<<(QDataStream &stream, const Akonadi::Scope &scope);
    friend QDataStream &::operator>>(QDataStream &stream, Akonadi::Scope &scope);
};

} // namespace Akonadi

#endif