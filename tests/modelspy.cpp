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

#include <kdebug.h>
#include <QTest>

ModelSpy::ModelSpy(QObject *parent)
    : QObject(parent), QList<QVariantList>(), m_isSpying(false)
{
  qRegisterMetaType<QModelIndex>("QModelIndex");
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

void ModelSpy::verifySignal( SignalType type, const QModelIndex& parent, int start, int end )
{
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QVERIFY( type == expectedSignal.signalType );
  QVERIFY( parent.data() == expectedSignal.parentData );
  QVERIFY( start == expectedSignal.startRow );
  QVERIFY( end == expectedSignal.endRow );
  if ( !expectedSignal.newData.isEmpty() )
  {
    // TODO
  }
}

void ModelSpy::verifySignal( SignalType type, const QModelIndex& parent, int start, int end, const QModelIndex& destParent, int destStart )
{
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QVERIFY( type == expectedSignal.signalType );
  QVERIFY( expectedSignal.startRow == start );
  QVERIFY( expectedSignal.endRow == end );
  QVERIFY( parent.data() == expectedSignal.sourceParentData );
  QVERIFY( destParent.data() == expectedSignal.parentData );
  QVERIFY( destStart == expectedSignal.destRow );
  QVariantList moveList;
  QModelIndex _parent = type == RowsAboutToBeMoved ? parent : destParent;
  int _start = type == RowsAboutToBeMoved ? start : destStart;
  int _end = type == RowsAboutToBeMoved ? end : destStart + (end - start);
  for ( int row = _start; row <= _end; ++row )
    moveList << m_model->index( row, 0, _parent ).data();
  QVERIFY( moveList == expectedSignal.newData );
}

void ModelSpy::verifySignal( SignalType type, const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
  Q_ASSERT( type == DataChanged );
  ExpectedSignal expectedSignal = m_expectedSignals.takeFirst();
  QVERIFY( type == expectedSignal.signalType );
  QModelIndex parent = topLeft.parent();
  if ( expectedSignal.parentData.isValid() )
    QVERIFY( parent.data() == expectedSignal.parentData );
  QVERIFY( topLeft.row() == expectedSignal.startRow );
  QVERIFY( bottomRight.row() == expectedSignal.endRow );
  for ( int i = 0, row = topLeft.row(); row <= bottomRight.row(); ++row, ++i )
  {
    QVERIFY( expectedSignal.newData.at( i ) == m_model->index( row, 0, parent ).data() );
  }
}

void ModelSpy::startSpying()
{
  m_isSpying = true;

  // If a signal is connected to a slot multiple times, the slot gets called multiple times.
  // As we're doing start and stop spying all the time, we disconnect here first to make sure.

  disconnect(m_model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsAboutToBeInserted(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int,const QModelIndex &, int)),
          this, SLOT(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  disconnect(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          this, SLOT(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  disconnect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));


  connect(m_model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          SLOT(rowsAboutToBeInserted(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          SLOT(rowsInserted(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          SLOT(rowsRemoved(const QModelIndex &, int, int)));
  connect(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int,const QModelIndex &, int)),
          SLOT(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  connect(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          SLOT(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  connect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));

}


void ModelSpy::stopSpying()
{
  m_isSpying = false;
  disconnect(m_model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsAboutToBeInserted(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
  disconnect(m_model, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int,const QModelIndex &, int)),
          this, SLOT(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  disconnect(m_model, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
          this, SLOT(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));

  disconnect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));

}

void ModelSpy::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsAboutToBeInserted << QVariant::fromValue(parent) << start << end);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeInserted, parent, start, end );
}

void ModelSpy::rowsInserted(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsInserted << QVariant::fromValue(parent) << start << end);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsInserted, parent, start, end );
}

void ModelSpy::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsAboutToBeRemoved << QVariant::fromValue(parent) << start << end);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeRemoved, parent, start, end );
}

void ModelSpy::rowsRemoved(const QModelIndex &parent, int start, int end)
{
  append(QVariantList() << RowsRemoved << QVariant::fromValue(parent) << start << end);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsRemoved, parent, start, end );
}

void ModelSpy::rowsAboutToBeMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  append(QVariantList() << RowsAboutToBeMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsAboutToBeMoved, srcParent, start, end, destParent, destStart );
}

void ModelSpy::rowsMoved(const QModelIndex &srcParent, int start, int end, const QModelIndex &destParent, int destStart)
{
  append(QVariantList() << RowsMoved << QVariant::fromValue(srcParent) << start << end << QVariant::fromValue(destParent) << destStart);
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( RowsMoved, srcParent, start, end, destParent, destStart );
}

void ModelSpy::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  append(QVariantList() << DataChanged << QVariant::fromValue(topLeft) << QVariant::fromValue(bottomRight));
  if ( !m_expectedSignals.isEmpty() )
    verifySignal( DataChanged, topLeft, bottomRight );
}

