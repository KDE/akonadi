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

#ifndef AKONADI_SCOPE_H
#define AKONADI_SCOPE_H

#include "libs/imapset_p.h"
#include <QStringList>

namespace Akonadi {
namespace Server {

class ImapStreamParser;

/**
  Represents a set of Akonadi objects (eg. items or collections) selected for an operations.
*/
class Scope
{
public:
    enum SelectionScope {
        Invalid,
        None,
        Uid,
        Rid,
        HierarchicalRid,
        Gid
    };

    /**
     * Constructs an invalid scope.
     *
     * This exists only so that we can register Scope as a metatype
     */
    Scope();

    Scope(SelectionScope scope);
    /**
      Parse the object set dependent on the set selection scope.
      The set has to be non-empty. If not a HandlerException is thrown.
    */
    void parseScope(ImapStreamParser *parser);

    /**
      Parse the selection scope identifier (UID, RID, HRID, etc.).
      Returns None if @p input is not a selection scope.
    */
    static SelectionScope selectionScopeFromByteArray(const QByteArray &input);

    SelectionScope scope() const;
    void setScope(SelectionScope scope);
    ImapSet uidSet() const;
    void setUidSet(const ImapSet &uidSet);
    QStringList ridSet() const;
    QStringList ridChain() const;
    QStringList gidSet() const;

private:
    SelectionScope mScope;
    ImapSet mUidSet;
    QStringList mRidSet;
    QStringList mRidChain;
    QStringList mGidSet;
};

} // namespace Server
} // namespace Akonadi

#endif
