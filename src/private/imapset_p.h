/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiprivate_export.h"

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QMetaType>
#include <QSharedDataPointer>

namespace Akonadi
{
namespace Protocol
{
class DataStream;
}
}

namespace Akonadi
{
class ImapIntervalPrivate;

/**
  Represents a single interval in an ImapSet.
  This class is implicitly shared.
*/
class AKONADIPRIVATE_EXPORT ImapInterval
{
public:
    /**
     * Describes the ids stored in the interval.
     */
    using Id = qint64;

    /**
      A list of ImapInterval objects.
    */
    using List = QList<ImapInterval>;

    /**
      Constructs an interval that covers all positive numbers.
    */
    ImapInterval();

    /**
      Copy constructor.
    */
    ImapInterval(const ImapInterval &other);

    /**
      Create a new interval.
      @param begin The begin of the interval.
      @param end Keep default (0) to just set the interval begin
    */
    explicit ImapInterval(Id begin, Id end = 0);

    /**
      Destructor.
    */
    ~ImapInterval();

    /**
      Assignment operator.
    */
    ImapInterval &operator=(const ImapInterval &other);

    /**
      Comparison operator.
    */
    bool operator==(const ImapInterval &other) const;

    /**
      Returns the size of this interval.
      Size is only defined for finite intervals.
    */
    Id size() const;

    /**
      Returns true if this interval has a defined begin.
    */
    bool hasDefinedBegin() const;

    /**
      Returns the begin of this interval. The value is the smallest value part of the interval.
      Only valid if begin is defined.
    */
    Id begin() const;

    /**
      Returns true if this intercal has been defined.
    */
    bool hasDefinedEnd() const;

    /**
      Returns the end of this interval. This value is the largest value part of the interval.
      Only valid if hasDefinedEnd() returned true.
    */
    Id end() const;

    /**
      Sets the begin of the interval.
    */
    void setBegin(Id value);

    /**
      Sets the end of this interval.
    */
    void setEnd(Id value);

    /**
      Converts this set into an IMAP compatible sequence.
    */
    QByteArray toImapSequence() const;

private:
    QSharedDataPointer<ImapIntervalPrivate> d;

    friend Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::ImapInterval &interval);
    friend Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::ImapInterval &interval);
};

class ImapSetPrivate;

/**
  Represents a set of natural numbers (1->\f$\infty\f$) in a as compact as possible form.
  Used to address Akonadi items via the IMAP protocol or in the database.
  This class is implicitly shared.
*/
class AKONADIPRIVATE_EXPORT ImapSet
{
public:
    /**
     * Describes the ids stored in the set.
     */
    using Id = qint64;

    /**
      Constructs an empty set.
    */
    ImapSet();

    ImapSet(qint64 Id); // krazy:exclude=explicit

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ImapSet(const QVector<qint64> &ids); // krazy:exclude=explicit
#endif

    ImapSet(const QList<qint64> &ids); // krazy:exclude=explicit

    ImapSet(const ImapInterval &interval); // krazy:exclude=explicit

    /**
      Copy constructor.
    */
    ImapSet(const ImapSet &other);

    /**
      Destructor.
    */
    ~ImapSet();

    /**
     * Returns ImapSet representing 1:*
     * */
    static ImapSet all();

    /**
      Assignment operator.
    */
    ImapSet &operator=(const ImapSet &other);

    bool operator==(const ImapSet &other) const;

    /**
      Adds the given list of positive integer numbers to the set.
      The list is sorted and split into as large as possible intervals.
      No interval merging is performed.
      @param values List of positive integer numbers in arbitrary order
    */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void add(const QVector<Id> &values);
#endif
    void add(const QList<Id> &values);

    /**
     * @overload
     */
    void add(const QSet<Id> &values);

    /**
      Adds the given ImapInterval to this set.
      No interval merging is performed.
    */
    void add(const ImapInterval &interval);

    /**
      Returns a IMAP-compatible QByteArray representation of this set.
    */
    QByteArray toImapSequenceSet() const;

    /**
      Returns the intervals this set consists of.
    */
    ImapInterval::List intervals() const;

    /**
      Returns true if this set doesn't contains any values.
    */
    bool isEmpty() const;

private:
    QSharedDataPointer<ImapSetPrivate> d;

    friend Protocol::DataStream &operator<<(Protocol::DataStream &stream, const Akonadi::ImapSet &set);
    friend Protocol::DataStream &operator>>(Protocol::DataStream &stream, Akonadi::ImapSet &set);
};

}

AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug d, const Akonadi::ImapInterval &interval);
AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug d, const Akonadi::ImapSet &set);

Q_DECLARE_TYPEINFO(Akonadi::ImapInterval, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::ImapSet, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::ImapInterval)
Q_DECLARE_METATYPE(Akonadi::ImapInterval::List)
Q_DECLARE_METATYPE(Akonadi::ImapSet)

