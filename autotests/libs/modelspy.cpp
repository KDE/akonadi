/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "modelspy.h"

#include <QTest>

QVariantList extractModelColumn(const QAbstractItemModel &model, const QModelIndex &parent, const int firstRow, const int lastRow)
{
    QVariantList result;
    for (auto row = firstRow; row <= lastRow; ++row) {
        result.append(model.index(row, 0, parent).data());
    }
    return result;
}

ModelSpy::ModelSpy(QAbstractItemModel *model, QObject *parent)
    : QObject{parent}, m_model{model}, m_isSpying{false}
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
}

bool ModelSpy::isEmpty() const
{
    return QList<QVariantList>::isEmpty() && m_expectedSignals.isEmpty();
}

void ModelSpy::setExpectedSignals(const QList< ExpectedSignal > &expectedSignals)
{
    m_expectedSignals = expectedSignals;
}

QList<ExpectedSignal> ModelSpy::expectedSignals() const
{
    return m_expectedSignals;
}

void ModelSpy::verifySignal(SignalType type, const QModelIndex &parent, int start, int end)
{
    const auto expectedSignal = m_expectedSignals.takeFirst();
    QCOMPARE(int(type), int(expectedSignal.signalType));
    QCOMPARE(parent.data(), expectedSignal.parentData);
    QCOMPARE(start, expectedSignal.startRow);
    QCOMPARE(end, expectedSignal.endRow);
    if (!expectedSignal.newData.isEmpty()) {
        // TODO
    }
}

void ModelSpy::verifySignal(SignalType type, const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destStart)
{
    const auto expectedSignal = m_expectedSignals.takeFirst();
    QCOMPARE(int(type), int(expectedSignal.signalType));
    QCOMPARE(start, expectedSignal.startRow);
    QCOMPARE(end, expectedSignal.endRow);
    QCOMPARE(parent.data(), expectedSignal.sourceParentData);
    QCOMPARE(destParent.data(), expectedSignal.parentData);
    QCOMPARE(destStart, expectedSignal.destRow);
    if (type == RowsAboutToBeMoved) {
        QCOMPARE(extractModelColumn(*m_model, parent, start, end), expectedSignal.newData);
    } else {
        const auto destEnd = destStart + (end - start);
        QCOMPARE(extractModelColumn(*m_model, destParent, destStart, destEnd), expectedSignal.newData);
    }
}

void ModelSpy::verifySignal(SignalType type, const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QCOMPARE(type, DataChanged);
    const auto expectedSignal = m_expectedSignals.takeFirst();
    QCOMPARE(int(type), int(expectedSignal.signalType));
    QModelIndex parent = topLeft.parent();
    //This check won't work for toplevel indexes
    if (parent.isValid()) {
        if (expectedSignal.parentData.isValid()) {
            QCOMPARE(parent.data(), expectedSignal.parentData);
        }
    }
    QCOMPARE(topLeft.row(), expectedSignal.startRow);
    QCOMPARE(bottomRight.row(), expectedSignal.endRow);
    QCOMPARE(extractModelColumn(*m_model, parent, topLeft.row(), bottomRight.row()), expectedSignal.newData);
}

void ModelSpy::startSpying()
{
    m_isSpying = true;

    // If a signal is connected to a slot multiple times, the slot gets called multiple times.
    // As we're doing start and stop spying all the time, we disconnect here first to make sure.

    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeInserted,
               this, &ModelSpy::rowsAboutToBeInserted);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &ModelSpy::rowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
               this, &ModelSpy::rowsAboutToBeRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &ModelSpy::rowsRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeMoved,
               this, &ModelSpy::rowsAboutToBeMoved);
    disconnect(m_model, &QAbstractItemModel::rowsMoved,
               this, &ModelSpy::rowsMoved);

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &ModelSpy::dataChanged);

    connect(m_model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &ModelSpy::rowsAboutToBeInserted);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &ModelSpy::rowsInserted);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &ModelSpy::rowsAboutToBeRemoved);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &ModelSpy::rowsRemoved);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeMoved,
            this, &ModelSpy::rowsAboutToBeMoved);
    connect(m_model, &QAbstractItemModel::rowsMoved,
            this, &ModelSpy::rowsMoved);

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &ModelSpy::dataChanged);

}

void ModelSpy::stopSpying()
{
    m_isSpying = false;
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeInserted,
               this, &ModelSpy::rowsAboutToBeInserted);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &ModelSpy::rowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
               this, &ModelSpy::rowsAboutToBeRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &ModelSpy::rowsRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeMoved,
               this, &ModelSpy::rowsAboutToBeMoved);
    disconnect(m_model, &QAbstractItemModel::rowsMoved,
               this, &ModelSpy::rowsMoved);

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &ModelSpy::dataChanged);

}

void ModelSpy::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsAboutToBeInserted, parent, start, end);
    } else {
        append(QVariantList{RowsAboutToBeInserted, QVariant::fromValue(parent), start, end});
    }
}

void ModelSpy::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsInserted, parent, start, end);
    } else {
        append(QVariantList{RowsInserted, QVariant::fromValue(parent), start, end});
    }
}

void ModelSpy::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsAboutToBeRemoved, parent, start, end);
    } else {
        append(QVariantList{RowsAboutToBeRemoved, QVariant::fromValue(parent), start, end});
    }
}

void ModelSpy::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsRemoved, parent, start, end);
    } else {
        append(QVariantList{RowsRemoved, QVariant::fromValue(parent), start, end});
    }
}

void ModelSpy::rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsAboutToBeMoved, srcParent, start, end, destParent, destStart);
    } else {
        append(QVariantList{RowsAboutToBeMoved, QVariant::fromValue(srcParent), start, end, QVariant::fromValue(destParent), destStart});
    }
}

void ModelSpy::rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(RowsMoved, srcParent, start, end, destParent, destStart);
    } else {
        append(QVariantList{RowsMoved, QVariant::fromValue(srcParent), start, end, QVariant::fromValue(destParent), destStart});
    }
}

void ModelSpy::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_expectedSignals.isEmpty()) {
        verifySignal(DataChanged, topLeft, bottomRight);
    } else {
        append(QVariantList{DataChanged, QVariant::fromValue(topLeft), QVariant::fromValue(bottomRight)});
    }
}

