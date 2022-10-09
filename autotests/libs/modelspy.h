/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonaditestfake_export.h"
#include <QModelIndex>
#include <QObject>
#include <QVariantList>

enum SignalType {
    NoSignal,
    RowsAboutToBeInserted,
    RowsInserted,
    RowsAboutToBeRemoved,
    RowsRemoved,
    RowsAboutToBeMoved,
    RowsMoved,
    DataChanged,
};

struct ExpectedSignal {
    ExpectedSignal(SignalType type, int start, int end, const QVariantList &newData)
        : ExpectedSignal{type, start, end, {}, newData}
    {
    }

    ExpectedSignal(SignalType type, int start, int end, const QVariant &parentData = {}, const QVariantList &newData = {})
        : signalType(type)
        , startRow(start)
        , endRow(end)
        , parentData(parentData)
        , newData(newData)
    {
    }

    ExpectedSignal(SignalType type,
                   int start,
                   int end,
                   const QVariant &sourceParentData,
                   int destRow,
                   const QVariant &destParentData,
                   const QVariantList &newData)
        : signalType(type)
        , startRow(start)
        , endRow(end)
        , parentData(destParentData)
        , sourceParentData(sourceParentData)
        , destRow(destRow)
        , newData(newData)
    {
    }

    SignalType signalType;
    int startRow;
    int endRow;
    QVariant parentData;
    QVariant sourceParentData;
    int destRow = 0;
    QVariantList newData;
};

Q_DECLARE_METATYPE(QModelIndex)

class AKONADITESTFAKE_EXPORT ModelSpy : public QObject, public QList<QVariantList>
{
    Q_OBJECT
public:
    explicit ModelSpy(QAbstractItemModel *model, QObject *parent = nullptr);

    bool isEmpty() const;

    void setExpectedSignals(const QList<ExpectedSignal> &expectedSignals);
    QList<ExpectedSignal> expectedSignals() const;

    void verifySignal(SignalType type, const QModelIndex &parent, int start, int end);
    void verifySignal(SignalType type, const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destStart);
    void verifySignal(SignalType type, const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void startSpying();
    void stopSpying();
    bool isSpying()
    {
        return m_isSpying;
    }

protected Q_SLOTS:
    void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart);
    void rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart);

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
    QAbstractItemModel *const m_model = nullptr;
    bool m_isSpying;
    QList<ExpectedSignal> m_expectedSignals;
};

#ifdef _MSC_VER
// FIXME: This is a very lousy implementation to make MSVC happy. Apparently
// MSVC insists on instantiating QSet<T> QList<T>::toSet() and complains about
// not having qHash() overload for QVariant(List) in QSet
//
// FIXME: This is good enough for ModelSpy, but should never ever be used in
// regular code.
inline uint qHash(const QVariant &v)
{
    return v.userType() + v.toUInt();
}

inline uint qHash(const QVariantList &list)
{
    return qHashRange(list.cbegin(), list.cend());
}
#endif
