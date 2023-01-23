/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cpphelper.h"
#include "nodetree.h"
#include "typehelper.h"

#include <QHash>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QSharedDataPointer>
#include <QSharedPointer>
namespace
{
class Dummy;

// FIXME: This is based on hacks and guesses, does not work for generated types
// and does not consider alignment. It should be good enough (TM) for our needs,
// but it would be nice to make it smarter, for example by looking up type sizes
// from the Node tree and understanding enums and QFlags types.
size_t typeSize(const QString &typeName)
{
    static QHash<QByteArray, size_t> typeSizeLookup = {{"Scope", sizeof(QSharedDataPointer<Dummy>)},
                                                       {"ScopeContext", sizeof(QSharedDataPointer<Dummy>)},
                                                       {"QSharedPointer", sizeof(QSharedPointer<Dummy>)},
                                                       {"Tristate", sizeof(qint8)},
                                                       {"Akonadi::Protocol::Attributes", sizeof(QMap<int, Dummy>)},
                                                       {"QSet", sizeof(QSet<Dummy>)},
                                                       {"QVector", sizeof(QVector<Dummy>)}};

    QByteArray tn;
    // Don't you just loooove hacks?
    // TODO: Extract underlying type during XML parsing
    if (typeName.startsWith(QLatin1String("Akonadi::Protocol")) && typeName.endsWith(QLatin1String("Ptr"))) {
        tn = "QSharedPointer";
    } else {
        tn = TypeHelper::isContainer(typeName) ? TypeHelper::containerName(typeName).toUtf8() : typeName.toUtf8();
    }
    auto it = typeSizeLookup.find(tn);
    if (it == typeSizeLookup.end()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const auto typeId = QMetaType::type(tn);
#else
        const auto typeId = QMetaType::fromName(tn).id();
#endif
        const int size = QMetaType(typeId).sizeOf();
        // for types of unknown size int
        it = typeSizeLookup.insert(tn, size ? size_t(size) : sizeof(int));
    }
    return *it;
}

} // namespace

void CppHelper::sortMembers(QVector<PropertyNode const *> &props)
{
    std::sort(props.begin(), props.end(), [](PropertyNode const *lhs, PropertyNode const *rhs) {
        return typeSize(lhs->type()) > typeSize(rhs->type());
    });
}

void CppHelper::sortMembersForSerialization(QVector<PropertyNode const *> &props)
{
    std::sort(props.begin(), props.end(), [](PropertyNode const *lhs, PropertyNode const *rhs) {
        return lhs->dependencies().isEmpty() > rhs->dependencies().isEmpty();
    });
}
