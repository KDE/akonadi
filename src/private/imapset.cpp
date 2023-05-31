/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "datastream_p_p.h"
#include "imapset_p.h"

#include <QSharedData>

#include <limits>

namespace Akonadi
{
class ImapIntervalPrivate : public QSharedData
{
public:
    ImapInterval::Id begin = 0;
    ImapInterval::Id end = 0;
};

class ImapSetPrivate : public QSharedData
{
public:
    template<typename T>
    void add(const T &values)
    {
        T vals = values;
        std::sort(vals.begin(), vals.end());
        for (int i = 0; i < vals.count(); ++i) {
            const int begin = vals[i];
            Q_ASSERT(begin >= 0);
            if (i == vals.count() - 1) {
                intervals << ImapInterval(begin, begin);
                break;
            }
            do {
                ++i;
                Q_ASSERT(vals[i] >= 0);
                if (vals[i] != (vals[i - 1] + 1)) {
                    --i;
                    break;
                }
            } while (i < vals.count() - 1);
            intervals << ImapInterval(begin, vals[i]);
        }
    }

    ImapInterval::List intervals;
};

ImapInterval::ImapInterval()
    : d(new ImapIntervalPrivate)
{
}

ImapInterval::ImapInterval(const ImapInterval &other)
    : d(other.d)
{
}

ImapInterval::ImapInterval(Id begin, Id end)
    : d(new ImapIntervalPrivate)
{
    d->begin = begin;
    d->end = end;
}

ImapInterval::~ImapInterval() = default;

ImapInterval &ImapInterval::operator=(const ImapInterval &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool ImapInterval::operator==(const ImapInterval &other) const
{
    return (d->begin == other.d->begin && d->end == other.d->end);
}

ImapInterval::Id ImapInterval::size() const
{
    if (!d->begin && !d->end) {
        return 0;
    }

    return (d->end - d->begin + 1);
}

bool ImapInterval::hasDefinedBegin() const
{
    return (d->begin != 0);
}

ImapInterval::Id ImapInterval::begin() const
{
    return d->begin;
}

bool ImapInterval::hasDefinedEnd() const
{
    return (d->end != 0);
}

ImapInterval::Id ImapInterval::end() const
{
    if (hasDefinedEnd()) {
        return d->end;
    }

    return std::numeric_limits<Id>::max();
}

void ImapInterval::setBegin(Id value)
{
    Q_ASSERT(value >= 0);
    Q_ASSERT(value <= d->end || !hasDefinedEnd());
    d->begin = value;
}

void ImapInterval::setEnd(Id value)
{
    Q_ASSERT(value >= 0);
    Q_ASSERT(value >= d->begin || !hasDefinedBegin());
    d->end = value;
}

QByteArray Akonadi::ImapInterval::toImapSequence() const
{
    if (size() == 0) {
        return QByteArray();
    }

    if (size() == 1) {
        return QByteArray::number(d->begin);
    }

    QByteArray rv = QByteArray::number(d->begin) + ':';

    if (hasDefinedEnd()) {
        rv += QByteArray::number(d->end);
    } else {
        rv += '*';
    }

    return rv;
}

ImapSet::ImapSet()
    : d(new ImapSetPrivate)
{
}

ImapSet::ImapSet(Id id)
    : d(new ImapSetPrivate)
{
    add(QList<Id>() << id);
}
ImapSet::ImapSet(const QList<qint64> &ids)
    : d(new ImapSetPrivate)
{
    add(ids);
}

ImapSet::ImapSet(const ImapInterval &interval)
    : d(new ImapSetPrivate)
{
    add(interval);
}

ImapSet::ImapSet(const ImapSet &other)
    : d(other.d)
{
}

ImapSet::~ImapSet()
{
}

ImapSet ImapSet::all()
{
    ImapSet set;
    set.add(ImapInterval(1, 0));
    return set;
}

ImapSet &ImapSet::operator=(const ImapSet &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool ImapSet::operator==(const ImapSet &other) const
{
    return d->intervals == other.d->intervals;
}

void ImapSet::add(const QList<Id> &values)
{
    d->add(values);
}

void ImapSet::add(const QSet<Id> &values)
{
    QList<Id> v;
    v.reserve(values.size());
    for (QSet<Id>::ConstIterator iter = values.constBegin(); iter != values.constEnd(); ++iter) {
        v.push_back(*iter);
    }

    add(v);
}

void ImapSet::add(const ImapInterval &interval)
{
    d->intervals << interval;
}

QByteArray ImapSet::toImapSequenceSet() const
{
    QByteArray rv;
    for (auto iter = d->intervals.cbegin(), end = d->intervals.cend(); iter != end; ++iter) {
        if (iter != d->intervals.cbegin()) {
            rv += ',';
        }
        rv += iter->toImapSequence();
    }

    return rv;
}

ImapInterval::List ImapSet::intervals() const
{
    return d->intervals;
}

bool ImapSet::isEmpty() const
{
    return d->intervals.isEmpty() || (d->intervals.size() == 1 && d->intervals.at(0).size() == 0);
}

Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::ImapInterval &interval)
{
    return stream << interval.d->begin << interval.d->end;
}

Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::ImapInterval &interval)
{
    return stream >> interval.d->begin >> interval.d->end;
}

Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::ImapSet &set)
{
    return stream << set.d->intervals;
}

Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::ImapSet &set)
{
    return stream >> set.d->intervals;
}

} // namespace Akonadi

using namespace Akonadi;

QDebug operator<<(QDebug d, const Akonadi::ImapInterval &interval)
{
    d << interval.toImapSequence();
    return d;
}

QDebug operator<<(QDebug d, const Akonadi::ImapSet &set)
{
    d << set.toImapSequenceSet();
    return d;
}
