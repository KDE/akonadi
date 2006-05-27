/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "collection.h"

using namespace Akonadi;

Collection::Collection( const QString& identifier )
    : m_identifier( identifier )
    , m_noSelect( false )
    , m_noInferiors( false )
{
}

Collection::~Collection( )
{
}

bool Collection::isNoSelect() const
{
    return m_noSelect;
}

bool Collection::isNoInferiors() const
{
    return m_noInferiors;
}

QString Collection::identifier() const
{
    return m_identifier;
}

void Akonadi::Collection::setNoSelect( bool v )
{
    m_noSelect = v;
}

void Akonadi::Collection::setNoInferiors( bool v )
{
    m_noInferiors = v;
}

void Akonadi::Collection::setMimeTypes( const QByteArray& mimetypes )
{
    m_mimeTypes = mimetypes;
}

QByteArray Akonadi::Collection::getMimeTypes() const
{
    return m_mimeTypes;
}

