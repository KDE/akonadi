/*
 *  SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *  SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "datastream_p_p.h"
#include "scope_p.h"
#include "shared/akranges.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

using namespace AkRanges;

namespace Akonadi
{
class ScopePrivate : public QSharedData
{
public:
    QList<qint64> uidSet;
    QStringList ridSet;
    QList<Scope::HRID> hridChain;
    QStringList gidSet;
    Scope::SelectionScope scope = Scope::Invalid;
};

Scope::HRID::HRID()
    : id(-1)
{
}

Scope::HRID::HRID(qint64 id, const QString &remoteId)
    : id(id)
    , remoteId(remoteId)
{
}

bool Scope::HRID::isEmpty() const
{
    return id <= 0 && remoteId.isEmpty();
}

void Scope::HRID::toJson(QJsonObject &json) const
{
    json[QStringLiteral("ID")] = id;
    json[QStringLiteral("RemoteID")] = remoteId;
}

Scope::Scope()
    : d(new ScopePrivate)
{
}

Scope::Scope(const Scope &) = default;
Scope::Scope(Scope &&) noexcept = default;

Scope::Scope(std::initializer_list<qint64> ids)
    : d(new ScopePrivate)
{
    setUidSet(std::move(ids));
}

Scope::Scope(qint64 id)
    : d(new ScopePrivate)
{
    setUidSet({id});
}

Scope::Scope(const QList<qint64> &uidSet)
    : d(new ScopePrivate)
{
    setUidSet(uidSet);
}

Scope::Scope(SelectionScope scope, const QStringList &ids)
    : d(new ScopePrivate)
{
    Q_ASSERT(scope == Rid || scope == Gid);
    if (scope == Rid) {
        d->scope = scope;
        d->ridSet = ids;
    } else if (scope == Gid) {
        d->scope = scope;
        d->gidSet = ids;
    }
}

Scope::Scope(const QList<HRID> &hrid)
    : d(new ScopePrivate)
{
    d->scope = HierarchicalRid;
    d->hridChain = hrid;
}

Scope::~Scope() = default;

Scope &Scope::operator=(const Scope &) = default;
Scope &Scope::operator=(Scope &&) noexcept = default;

bool Scope::operator==(const Scope &other) const
{
    if (d->scope != other.d->scope) {
        return false;
    }

    switch (d->scope) {
    case Uid:
        return d->uidSet == other.d->uidSet;
    case Gid:
        return d->gidSet == other.d->gidSet;
    case Rid:
        return d->ridSet == other.d->ridSet;
    case HierarchicalRid:
        return d->hridChain == other.d->hridChain;
    case Invalid:
        return true;
    }

    Q_ASSERT(false);
    return false;
}

bool Scope::operator!=(const Scope &other) const
{
    return !(*this == other);
}

Scope::SelectionScope Scope::scope() const
{
    return d->scope;
}

bool Scope::isEmpty() const
{
    switch (d->scope) {
    case Invalid:
        return true;
    case Uid:
        return d->uidSet.isEmpty();
    case Rid:
        return d->ridSet.isEmpty();
    case HierarchicalRid:
        return d->hridChain.isEmpty();
    case Gid:
        return d->gidSet.isEmpty();
    }

    Q_ASSERT(false);
    return true;
}

void Scope::setUidSet(const QList<qint64> &uidSet)
{
    d->scope = Uid;
    d->uidSet = uidSet;
}

QList<qint64> Scope::uidSet() const
{
    return d->uidSet;
}

void Scope::setRidSet(const QStringList &ridSet)
{
    d->scope = Rid;
    d->ridSet = ridSet;
}

QStringList Scope::ridSet() const
{
    return d->ridSet;
}

void Scope::setHRidChain(const QList<HRID> &hridChain)
{
    d->scope = HierarchicalRid;
    d->hridChain = hridChain;
}

QList<Scope::HRID> Scope::hridChain() const
{
    return d->hridChain;
}

void Scope::setGidSet(const QStringList &gidSet)
{
    d->scope = Gid;
    d->gidSet = gidSet;
}

QStringList Scope::gidSet() const
{
    return d->gidSet;
}

qint64 Scope::uid() const
{
    if (d->uidSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(d->uidSet.size() == 1);
        return -1;
    }

    return d->uidSet.front();
}

QString Scope::rid() const
{
    if (d->ridSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(d->ridSet.size() == 1);
        return QString();
    }
    return d->ridSet.at(0);
}

QString Scope::gid() const
{
    if (d->gidSet.size() != 1) {
        // TODO: Error handling!
        Q_ASSERT(d->gidSet.size() == 1);
        return QString();
    }
    return d->gidSet.at(0);
}

void Scope::toJson(QJsonObject &json) const
{
    switch (scope()) {
    case Scope::Uid:
        json[QStringLiteral("type")] = QStringLiteral("UID");
        json[QStringLiteral("value")] = QJsonArray::fromVariantList(uidSet() | Views::transform([](qint64 id) {
                                                                        return QVariant(id);
                                                                    })
                                                                    | Actions::toQList);
        break;
    case Scope::Rid:
        json[QStringLiteral("type")] = QStringLiteral("RID");
        json[QStringLiteral("value")] = QJsonArray::fromStringList(ridSet());
        break;
    case Scope::Gid:
        json[QStringLiteral("type")] = QStringLiteral("GID");
        json[QStringLiteral("value")] = QJsonArray::fromStringList(gidSet());
        break;
    case Scope::HierarchicalRid: {
        const auto &chain = hridChain();
        QJsonArray hridArray;
        for (const auto &hrid : chain) {
            QJsonObject obj;
            hrid.toJson(obj);
            hridArray.append(obj);
        }
        json[QStringLiteral("type")] = QStringLiteral("HRID");
        json[QStringLiteral("value")] = hridArray;
    } break;
    default:
        json[QStringLiteral("type")] = QStringLiteral("invalid");
        json[QStringLiteral("value")] = QJsonValue(static_cast<int>(scope()));
    }
}

Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::Scope &scope)
{
    stream << static_cast<quint8>(scope.d->scope);
    switch (scope.d->scope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream << scope.d->uidSet;
        return stream;
    case Scope::Rid:
        stream << scope.d->ridSet;
        return stream;
    case Scope::HierarchicalRid:
        stream << scope.d->hridChain;
        return stream;
    case Scope::Gid:
        stream << scope.d->gidSet;
        return stream;
    }

    return stream;
}

Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::Scope::HRID &hrid)
{
    return stream << hrid.id << hrid.remoteId;
}

Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::Scope::HRID &hrid)
{
    return stream >> hrid.id >> hrid.remoteId;
}

Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::Scope &scope)
{
    scope.d->uidSet.clear();
    scope.d->ridSet.clear();
    scope.d->hridChain.clear();
    scope.d->gidSet.clear();

    stream >> reinterpret_cast<quint8 &>(scope.d->scope);
    switch (scope.d->scope) {
    case Scope::Invalid:
        return stream;
    case Scope::Uid:
        stream >> scope.d->uidSet;
        return stream;
    case Scope::Rid:
        stream >> scope.d->ridSet;
        return stream;
    case Scope::HierarchicalRid:
        stream >> scope.d->hridChain;
        return stream;
    case Scope::Gid:
        stream >> scope.d->gidSet;
        return stream;
    }

    return stream;
}

} // namespace Akonadi

using namespace Akonadi;

QDebug operator<<(QDebug dbg, const Akonadi::Scope::HRID &hrid)
{
    return dbg.nospace() << "(ID: " << hrid.id << ", RemoteID: " << hrid.remoteId << ")";
}

QDebug operator<<(QDebug dbg, const Akonadi::Scope &scope)
{
    switch (scope.scope()) {
    case Scope::Uid:
        return dbg.nospace() << "Scope(UID, " << scope.uidSet() << ")";
    case Scope::Rid:
        return dbg.nospace() << "Scope(RID, " << scope.ridSet() << ")";
    case Scope::Gid:
        return dbg.nospace() << "Scope(GID, " << scope.gidSet() << ")";
    case Scope::HierarchicalRid:
        return dbg.nospace() << "Scope(HRID, " << scope.hridChain() << ")";
    default:
        return dbg.nospace() << "Scope(Invalid)";
    }
}
