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

#include <qdebug.h>

ModelSpy::ModelSpy(QObject *parent)
    : QObject(parent), QList<QVariantList>(), m_isSpying(false)
{
  qRegisterMetaType<QModelIndex>("QModelIndex");
}

bool ModelSpy::isEmpty() const
{
  return QList<QVariantList>::isEmpty() && m_expectedSignals.isEmpty();
}

void ModelSpy::setModel(QAbstractItemModel *model)
{
  Q_ASSERT(model);
  m_model = model;
}

void ModelSpy::setExpectedSignals( QList< ExpectedSignal > expectedSignals )
{
  m_expectedSignals = expectedSignals;
}

QList<ExpectedSignal> ModelSpy::expectedSignals() const
{
  return m_expectedSignals;
}

void ModelSpy::verifySignal( SignalType type, const QModelIndex& parent, int start, int end )
{
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QCOMPARE( int(type), int(expectedSignal.signalType) );
  QCOMPARE( parent.data(), expectedSignal.parentData );
  QCOMPARE( start, expectedSignal.startRow );
  QCOMPARE( end, expectedSignal.endRow );
  if ( !expectedSignal.newData.isEmpty() )
  {
    // TODO
  }
}

void ModelSpy::verifySignal( SignalType type, const QModelIndex& parent, int start, int end, const QModelIndex& destParent, int destStart )
{
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QCOMPARE( int(type), int(expectedSignal.signalType) );
  QCOMPARE( expectedSignal.startRow, start );
  QCOMPARE( expectedSignal.endRow, end );
  QCOMPARE( parent.data(), expectedSignal.sourceParentData );
  QCOMPARE( destParent.data(), expectedSignal.parentData );
  QCOMPARE( destStart, expectedSignal.destRow );
  QVariantList moveList;
  QModelIndex _parent = type == RowsAboutToBeMoved ? parent : destParent;
  int _start = type == RowsAboutToBeMoved ? start : destStart;
  int _end = type == RowsAboutToBeMoved ? end : destStart + (end - start);
  for ( int row = _start; row <= _end; ++row )
    moveList << m_model->index( row, 0, _parent ).data();
  QCOMPARE( moveList, expectedSignal.newData );
}

void ModelSpy::verifySignal( SignalType type, const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
  QCOMPARE( type, DataChanged );
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QCOMPARE( int(type), int(expectedSignal.signalType) );
  QModelIndex parent = topLeft.parent();
  //This check won't work for toplevel indexes
  if ( parent.isValid() ) {
    if ( expectedSignal.parentData.isValid() )
      QCOMPARE( parent.data(), expectedSignal.parentData );
  }
  QCOMPARE( topLeft.row(), expectedSignal.startRow );
  QCOMPARE( bottomRight.row(), expectedSignal.endRow );
  for ( int i = 0, row = topLeft.row(); row <= bottomRight.row(); ++row, ++i )
  {
    QCOMPARE( expectedSignal.newData.at( i ), m_model->index( row, 0, parent ).data() );
  }
}

void ModelSpy::startSpying()
{
  m_isSpying = true;

  // If a signal is connected to a slot multiple times, the slot gets called multiple times.
  // As we're doing start and stop spying all the time, we disconnect here first to make sure.

  disconnect(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
          this, SLOT(rowsInserted(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
          this, SLOT(rowsRemoved(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
          this, SLOT(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
  disconnect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));

  disconnect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this, SLOT(dataChanged(QModelIndex,QModelIndex)));

  connect(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
          SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
  connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
          SLOT(rowsInserted(QModelIndex,int,int)));
  connect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
          SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
  connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
          SLOT(rowsRemoved(QModelIndex,int,int)));
  connect(m_model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
          SLOT(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
  connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));

  connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          SLOT(dataChanged(QModelIndex,QModelIndex)));

}

void ModelSpy::stopSpying()
{
  m_isSpying = false;
  disconnect(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeInserted(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)),
          this, SLOT(rowsInserted(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
          this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
          this, SLOT(rowsRemoved(QModelIndex,int,int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
          this, SLOT(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
  disconnect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
          this, SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));

  disconnect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          this, SLOT(dataChanged(QModelIndex,QModelIndex)));

}

void ModelSpy::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeInserted, parent, start, end );
  else
    append(QVariantList() << RowsAboutToBeInserted << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsInserted(const QModelIndex &parent, int start, int end)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsInserted, parent, start, end );
  else
    append(QVariantList() << RowsInserted << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeRemoved, parent, start, end );
  else
    append(QVariantList() << RowsAboutToBeRemoved << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsRemoved(const QModelIndex &parent, int start, int end)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsRemoved, parent, start, end );
  else
    append(QVariantList() << RowsRemoved << QVariant::fromValue(parent) << start << end);
}

void ModelSpy::rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeMoved, srcParent, start, end, destParent, destStart );
  else
    append(QVariantList() << RowsAboutToBeMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
}

void ModelSpy::rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsMoved, srcParent, start, end, destParent, destStart );
  else
    append(QVariantList() << RowsMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
}

void ModelSpy::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( DataChanged, topLeft, bottomRight );
  else
    append(QVariantList() << DataChanged << QVariant::fromValue(topLeft) << QVariant::fromValue(bottomRight));
}

