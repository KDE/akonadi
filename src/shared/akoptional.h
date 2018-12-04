/*
    Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKOPTIONAL_H
#define AKOPTIONAL_H

#include "config-akonadi.h"

#ifdef WITH_3RDPARTY_OPTIONAL
    // Unlike std::experimental::optional<> from GCC, this one provides
    // full interface described in N3793
    #include "optional.hpp" // from 3rdparty/Optional
    namespace Akonadi {
        template<typename T>
        using akOptional = std::experimental::optional<T>;
        using nullopt_t = std::experimental::nullopt_t;
        constexpr nullopt_t nullopt{nullopt_t::init()};
    }
#else
    #include <optional>
    namespace Akonadi {
        template<typename T>
        using akOptional = std::optional<T>;
        using nullopt_t = std::nullopt_t;
        constexpr nullopt_t nullopt{nullopt_t::init()};
    }
#endif

#endif // AKOPTIONAL_H
