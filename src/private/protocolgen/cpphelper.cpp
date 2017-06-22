/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.og>

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

#include "cpphelper.h"
#include "typehelper.h"
#include "nodetree.h"

#include <QHash>
#include <QSharedDataPointer>
#include <QMap>
#include <QMetaType>
#include <QDebug>

namespace {

class Dummy;

static QHash<QByteArray, size_t> typeSizeLookup = {
    { "Scope",        sizeof(QSharedDataPointer<Dummy>) },
    { "ScopeContext", sizeof(QSharedDataPointer<Dummy>) },
    { "QSharedPointer", sizeof(QSharedPointer<Dummy>) },
    { "Tristate",     sizeof(qint8) },
    { "Akonadi::Protocol::Attributes",   sizeof(QMap<int, Dummy>) },
    { "QSet",         sizeof(QSet<Dummy>) },
    { "QVector",      sizeof(QVector<Dummy>) }
};

// FIXME: This is based on hacks and guesses, does not work for generated types
// and does not consider alignment. It should be good enough (TM) for our needs,
// but it would be nice to make it smarter, for example by looking up type sizes
// from the Node tree and understanding enums and QFlags types.
size_t typeSize(const QString &typeName)
{
    QByteArray tn;
    // Don't you just loooove hacks?
    // TODO: Extract underlying type during XML parsing
    if (typeName.startsWith(QLatin1String("Akonadi::Protocol")) &&
        typeName.endsWith(QLatin1String("Ptr"))) {
        tn = "QSharedPointer";
    } else {
        tn = TypeHelper::isContainer(typeName)
                                ? TypeHelper::containerName(typeName).toUtf8()
                                : typeName.toUtf8();
    }
    auto it = typeSizeLookup.find(tn);
    if (it == typeSizeLookup.end()) {
        const auto typeId = QMetaType::type(tn);
        const int size = QMetaType(typeId).sizeOf();
        // for types of unknown size int
        it = typeSizeLookup.insert(tn, size ? size_t(size) : sizeof(int));
    }
    return *it;
}

}

void CppHelper::sortMembers(QVector<PropertyNode const *> &props)
{
    std::sort(props.begin(), props.end(),
              [](PropertyNode const *lhs, PropertyNode const *rhs) {
                  return typeSize(lhs->type()) > typeSize(rhs->type());
              });
}

void CppHelper::sortMembersForSerialization(QVector<PropertyNode const *> &props)
{
    std::sort(props.begin(), props.end(),
              [](PropertyNode const *lhs, PropertyNode const *rhs) {
                  return lhs->dependencies().isEmpty() > rhs->dependencies().isEmpty();
              });
}

