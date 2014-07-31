/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "schematypes.h"

#include <boost/bind.hpp>
#include <algorithm>

using namespace Akonadi::Server;

ColumnDescription::ColumnDescription()
    : size(-1)
    , allowNull(true)
    , isAutoIncrement(false)
    , isPrimaryKey(false)
    , isUnique(false)
    , onUpdate(Cascade)
    , onDelete(Cascade)
    , noUpdate(false)
{
}

IndexDescription::IndexDescription()
    : isUnique(false)
{
}

DataDescription::DataDescription()
{
}

TableDescription::TableDescription()
{
}

int TableDescription::primaryKeyColumnCount() const
{
    return std::count_if(columns.constBegin(), columns.constEnd(), boost::bind<bool>(&ColumnDescription::isPrimaryKey, _1));
}

RelationDescription::RelationDescription()
{
}
