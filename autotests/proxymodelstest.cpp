/*
    Copyright (c) 2010 Till Adam <till@kdab.com>

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

#include <qtest.h>


#include <krecursivefilterproxymodel.h>
#include <QStandardItemModel>

class KRFPTestModel : public KRecursiveFilterProxyModel
{
public:
    KRFPTestModel( QObject* parent ) : KRecursiveFilterProxyModel( parent ) { }

    bool acceptRow( int sourceRow, const QModelIndex& sourceParent ) const
    {
        const QModelIndex modelIndex = sourceModel()->index( sourceRow, 0, sourceParent );
        return !modelIndex.data().toString().contains("three");
    }

};

class ProxyModelsTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();

  void init();

  void testMatch();

private:
  QStandardItemModel m_model;
  KRecursiveFilterProxyModel*m_krfp;
  KRFPTestModel *m_krfptest;
};

void ProxyModelsTest::initTestCase()
{
}

void ProxyModelsTest::init()
{
  m_model.setRowCount( 5 );
  m_model.setColumnCount( 1  );
  m_model.setData( m_model.index( 0, 0, QModelIndex() ), "one" );
  QModelIndex idx = m_model.index( 1, 0, QModelIndex() );
  m_model.setData( idx, "two" );
  m_model.insertRows( 0, 1, idx );
  m_model.insertColumns( 0, 1, idx );
  m_model.setData( m_model.index( 0, 0, idx ), "three" );
  m_model.setData( m_model.index( 2, 0, QModelIndex() ), "three" );
  m_model.setData( m_model.index( 3, 0, QModelIndex() ), "four" );
  m_model.setData( m_model.index( 4, 0, QModelIndex() ), "five" );

  m_model.setData( m_model.index( 4, 0, QModelIndex() ), "mystuff", Qt::UserRole+42 );

  m_krfp = new KRecursiveFilterProxyModel(this);
  m_krfp->setSourceModel(&m_model);
  m_krfptest = new KRFPTestModel (this);
  m_krfptest->setSourceModel(m_krfp);

  // some sanity checks that setup worked
  QCOMPARE( m_model.rowCount( QModelIndex() ), 5 );
  QCOMPARE( m_model.data( m_model.index( 0, 0 ) ).toString(), QStringLiteral("one") );
  QCOMPARE( m_krfp->rowCount( QModelIndex() ), 5 );
  QCOMPARE( m_krfp->data( m_krfp->index( 0, 0 ) ).toString(), QStringLiteral("one") );
  QCOMPARE( m_krfptest->rowCount( QModelIndex() ), 4 );
  QCOMPARE( m_krfptest->data( m_krfptest->index( 0, 0 ) ).toString(), QStringLiteral("one") );

  QCOMPARE( m_krfp->rowCount( m_krfp->index( 1, 0 ) ), 1 );
  QCOMPARE( m_krfptest->rowCount( m_krfptest->index( 1, 0 ) ), 0 );
}

void ProxyModelsTest::testMatch()
{
    QModelIndexList results = m_model.match( m_model.index( 0, 0 ), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 1 );
    results = m_model.match( m_model.index( 0,0 ), Qt::DisplayRole, "fourtytwo" );
    QCOMPARE( results.size(), 0 );
    results = m_model.match( m_model.index( 0,0 ), Qt::UserRole+42, "mystuff" );
    QCOMPARE( results.size(), 1 );

    results = m_krfp->match( m_krfp->index( 0, 0 ), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 1 );
    results = m_krfp->match( m_krfp->index( 0,0 ), Qt::UserRole+42, "mystuff" );
    QCOMPARE( results.size(), 1 );

    results = m_krfptest->match( m_krfptest->index( 0, 0 ), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 0 );
    results = m_krfptest->match( m_krfptest->index( 0,0 ), Qt::UserRole+42, "mystuff" );
    QCOMPARE( results.size(), 1 );

    results = m_model.match( QModelIndex(), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 0 );
    results = m_krfp->match( QModelIndex(), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 0 );
    results = m_krfptest->match( QModelIndex(), Qt::DisplayRole, "three" );
    QCOMPARE( results.size(), 0 );

    const QModelIndex index = m_model.index( 0, 0, QModelIndex() );
    results = m_model.match( index, Qt::DisplayRole, "three", -1, Qt::MatchRecursive | Qt::MatchStartsWith | Qt::MatchWrap );
    QCOMPARE( results.size(), 2 );
}

#include "proxymodelstest.moc"

QTEST_MAIN(ProxyModelsTest)

