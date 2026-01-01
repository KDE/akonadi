/*
    SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QSet>

#include <algorithm>

namespace Akonadi
{
template<typename Key, typename Value, template<typename, typename> class Container>
QList<Value> valuesToVector(const Container<Key, Value> &container)
{
    QList<Value> values;
    values.reserve(container.size());
    for (const auto &value : container) {
        values.append(value);
    }
    return values;
}

template<typename T>
QSet<T> vectorToSet(const QList<T> &container)
{
    QSet<T> set;
    set.reserve(container.size());
    for (const auto &value : container) {
        set.insert(value);
    }
    return set;
}

template<typename Value, template<typename> class Container>
QList<Value> setToVector(const Container<Value> &container)
{
    QList<Value> values;
    values.reserve(container.size());
    for (const auto &value : container) {
        values.append(value);
    }
    return values;
}

} // namespace Akonadi
