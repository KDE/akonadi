/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "scope.h"
#include "imapstreamparser.h"
#include "handler.h"
#include "libs/protocol_p.h"

#include <cassert>

using namespace Akonadi;
using namespace Akonadi::Server;

Scope::Scope()
    : mScope(Scope::Invalid)
{
}

Scope::Scope(SelectionScope scope)
    : mScope(scope)
{
}

void Scope::parseScope(ImapStreamParser *parser)
{
    if (mScope == None || mScope == Uid) {
        mUidSet = parser->readSequenceSet();
        if (mUidSet.isEmpty()) {
            throw HandlerException("Empty uid set specified");
        }
    } else if (mScope == Rid) {
        if (parser->hasList()) {
            parser->beginList();
            while (!parser->atListEnd()) {
                mRidSet << parser->readUtf8String();
            }
        } else {
            mRidSet << parser->readUtf8String();
        }
        if (mRidSet.isEmpty()) {
            throw HandlerException("Empty remote identifier set specified");
        }
    } else if (mScope == HierarchicalRid) {
        parser->beginList();
        while (!parser->atListEnd()) {
            parser->beginList();
            parser->readString(); // uid, invalid here
            mRidChain.append(parser->readUtf8String());
            if (!parser->atListEnd()) {
                throw HandlerException("Invalid hierarchical RID chain format");
            }
        }
    } else if (mScope == Gid) {
        if (parser->hasList()) {
            parser->beginList();
            while (!parser->atListEnd()) {
                mGidSet << parser->readUtf8String();
            }
        } else {
            mGidSet << parser->readUtf8String();
        }
        if (mGidSet.isEmpty()) {
            throw HandlerException("Empty gid set specified");
        }
    } else {
        throw HandlerException("WTF?!?");
    }
}

Scope::SelectionScope Scope::selectionScopeFromByteArray(const QByteArray &input)
{
    if (input == AKONADI_CMD_UID) {
        return Scope::Uid;
    } else if (input == AKONADI_CMD_RID) {
        return Scope::Rid;
    } else if (input == AKONADI_CMD_HRID) {
        return Scope::HierarchicalRid;
    } else if (input == AKONADI_CMD_GID) {
        return Scope::Gid;
    }
    return Scope::None;
}

Scope::SelectionScope Scope::scope() const
{
    return mScope;
}

void Scope::setScope(SelectionScope scope)
{
    mScope = scope;
}

ImapSet Scope::uidSet() const
{
    return mUidSet;
}

void Scope::setUidSet(const ImapSet &uidSet)
{
    mUidSet = uidSet;
}

QStringList Scope::ridSet() const
{
    return mRidSet;
}

QStringList Scope::ridChain() const
{
    return mRidChain;
}

QStringList Scope::gidSet() const
{
    return mGidSet;
}
