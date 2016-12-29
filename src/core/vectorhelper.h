/*
    Copyright (c) 2015 Laurent Montel <montel@kde.org>

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

#ifndef VECTORHELPER_H
#define VECTORHELPER_H

#include <QVector>
#include <QSet>

namespace Akonadi {
template<typename Key, typename Value, template<typename, typename> class Container>
QVector<Value> valuesToVector(const Container<Key, Value> &container)
{
    QVector<Value> values;
    values.reserve(container.size());
    for (const Value &value : container) {
        values << value;
    }
    return values;
}

template<typename T>
QSet<T> vectorToSet(const QVector<T> &container)
{
    QSet<T> set;
    set.reserve(container.size());
    for (const T &value : container) {
        set.insert(value);
    }
    return set;
}

}

#endif // VECTORHELPER_H
