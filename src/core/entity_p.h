/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef ENTITY_P_H
#define ENTITY_P_H

#include "collection.h"

Q_GLOBAL_STATIC(Akonadi::Collection, s_defaultParentCollection)

#define AKONADI_DEFINE_PRIVATE( Class ) \
    Class##Private *Class ::d_func() \
    {\
        return reinterpret_cast<Class##Private *>( d_ptr.data() );\
    } \
    const Class##Private *Class ::d_func() const \
    {\
        return reinterpret_cast<const Class##Private *>( d_ptr.data() );\
    }

#endif
