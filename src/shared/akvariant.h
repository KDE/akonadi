/*
    Copyright (C) 2019 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKVARIANT_H
#define AKVARIANT_H

#include "config-akonadi.h"

#ifdef WITH_3RDPARTY_VARIANT
    #include "variant.hpp" // from 3rdparty/MPark.Variant
    namespace AkVariant
    {
        // Injects content of the mpark namspace into AkVariant namespace
        using namespace mpark;
    }
#else
    #include <variant>
    namespace AkVariant
    {
        // FIXME: this virtually aliases std for AkVariant
        using namespace std;
    }
#endif

namespace AkVariant
{

template<class ... Fs>
struct overloaded;

template<class Head, class ... Tail>
struct overloaded<Head, Tail ...> : Head, overloaded<Tail ...>
{
    overloaded(Head head, Tail ... tail)
        : Head(head), overloaded<Tail ...>(tail ...) {}

    using Head::operator();
    using overloaded<Tail ...>::operator();
};

template<class Func>
struct overloaded<Func> : Func
{
    overloaded(Func func)
        : Func(func) {}

    using Func::operator();
};



template <class... Fs>
auto make_overload(Fs... fs)
{
    return overloaded<Fs...>(fs...);
}

} // AkVariant


#endif // AKVARIANT_H
