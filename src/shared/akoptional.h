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

#ifdef __has_include
    #if __has_include(<optional>)
        #include <optional>
        #if defined(_MSC_VER) && !defined(__cpp_lib_optional)
            // Pretend MSVC supports feature test macros
            #define __cpp_lib_optional 1
        #endif
    #endif
    #if !defined(__cpp_lib_optional) && __has_include(<experimental/optional>)
        #include <experimental/optional>
        namespace std { using namespace experimental; }
    #else
        #error Compiler does not support std::optional or std::experimental::optional
    #endif
#else
    #error Compiler does not support __has_include
#endif

#endif // AKOPTIONAL_H
