/*
   Copyright (C) 2008-2009 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * This file looks very weird but is the only way to get the Query metatype
 * into the service interface when generated via qdbusxml2cpp.
 */

#ifndef _NEPOMUK_QUERY_META_TYPE_H_
#define _NEPOMUK_QUERY_META_TYPE_H_

#include "query.h"

Q_DECLARE_METATYPE(Nepomuk::Search::Query)

#endif
