/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SHAREDVALUEPOOL_P_H
#define AKONADI_SHAREDVALUEPOOL_P_H

#include <algorithm>

namespace Akonadi
{
namespace Internal
{

/**
 * Pool of implicitly shared values, use for optimizing memory use
 * when having a large amount of copies from a small set of different values.
 */
template <typename T, template <typename> class Container>
class SharedValuePool
{
public:
    /** Returns the shared value equal to @p value .*/
    T sharedValue(const T &value)
    {
        // for small pool sizes this is actually faster than using lower_bound and a sorted vector
        typename Container<T>::const_iterator it = std::find(m_pool.constBegin(), m_pool.constEnd(), value);
        if (it != m_pool.constEnd()) {
            return *it;
        }
        m_pool.push_back(value);
        return value;
    }

private:
    Container<T> m_pool;
};

}
}

#endif
