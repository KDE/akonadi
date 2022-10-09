/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <algorithm>

namespace Akonadi
{
namespace Internal
{
/**
 * Pool of implicitly shared values, use for optimizing memory use
 * when having a large amount of copies from a small set of different values.
 */
template<typename T, template<typename> class Container>
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
