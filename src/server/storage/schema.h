/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SCHEMA_H
#define SCHEMA_H

#include "schematypes.h"

namespace Akonadi
{
namespace Server
{

/** Methods to access the desired database schema (@see DbInspector for accessing
    the actual database schema).
 */
class Schema
{
public:
    inline virtual ~Schema()
    {
    }

    /** List of tables in the schema. */
    virtual QVector<TableDescription> tables() = 0;

    /** List of relations (N:M helper tables) in the schema. */
    virtual QVector<RelationDescription> relations() = 0;
};

} // namespace Server
} // namespace Akonadi

#endif
