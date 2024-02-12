/*
 *  SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *  SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// krazy:excludeall=dpointer

#pragma once

#include "akonadiprivate_export.h"

#include <QSharedDataPointer>

#include <QString>
class QJsonObject;
#include <QStringList>

namespace Akonadi
{
namespace Protocol
{
class DataStream;
}

class ScopePrivate;
class AKONADIPRIVATE_EXPORT Scope
{
public:
    enum SelectionScope : uchar {
        Invalid = 0,
        Uid = 1 << 0,
        Rid = 1 << 1,
        HierarchicalRid = 1 << 2,
        Gid = 1 << 3,
    };

    class AKONADIPRIVATE_EXPORT HRID
    {
    public:
        HRID();
        explicit HRID(qint64 id, const QString &remoteId = QString());
        HRID(const HRID &other) = default;
        HRID(HRID &&other) noexcept = default;

        HRID &operator=(const HRID &other) = default;
        HRID &operator=(HRID &&other) noexcept = default;

        ~HRID() = default;

        bool isEmpty() const;
        bool operator==(const HRID &other) const = default;

        void toJson(QJsonObject &json) const;

        qint64 id;
        QString remoteId;
    };

    explicit Scope();
    Scope(SelectionScope scope, const QStringList &ids);

    /* UID */
    Scope(std::initializer_list<qint64> ids);
    Scope(qint64 id); // krazy:exclude=explicit
    Scope(const QList<qint64> &ids); // krazy:exclude=explicit
    Scope(const QList<HRID> &hridChain); // krazy:exclude=explicit

    Scope(const Scope &other);
    Scope(Scope &&other) noexcept;
    ~Scope();

    Scope &operator=(const Scope &other);
    Scope &operator=(Scope &&other) noexcept;

    bool operator==(const Scope &other) const;
    bool operator!=(const Scope &other) const;

    SelectionScope scope() const;

    bool isEmpty() const;

    void setUidSet(const QList<qint64> &ids);
    QList<qint64> uidSet() const;

    void setRidSet(const QStringList &ridSet);
    QStringList ridSet() const;

    void setHRidChain(const QList<HRID> &ridChain);
    QList<HRID> hridChain() const;

    void setGidSet(const QStringList &gidChain);
    QStringList gidSet() const;

    qint64 uid() const;
    QString rid() const;
    QString gid() const;

    void toJson(QJsonObject &json) const;

private:
    QSharedDataPointer<ScopePrivate> d;
    friend class ScopePrivate;

    friend Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::Scope &scope);
    friend Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::Scope &scope);
};

} // namespace Akonadi

Q_DECLARE_TYPEINFO(Akonadi::Scope::HRID, Q_RELOCATABLE_TYPE);

AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug debug, const Akonadi::Scope &scope);
