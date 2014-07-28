/*
    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#include "itemhydratest.h"

#include <memory>
#include "item.h"
#include <qtest.h>
#include <QDebug>
#include <QSharedPointer>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

using namespace Akonadi;
using boost::shared_ptr;

struct Volker
{
    bool operator==( const Volker& f ) const
    {
        return f.who == who;
    }
    virtual ~Volker() { }
    virtual Volker * clone() const = 0;
    QString who;
};
typedef shared_ptr<Volker> VolkerPtr;
typedef QSharedPointer<Volker> VolkerQPtr;

struct Rudi: public Volker
{
    Rudi() { who = QLatin1String("Rudi"); }
    virtual ~Rudi() { }
    /* reimp */ Rudi * clone() const { return new Rudi( *this ); }
};

typedef shared_ptr<Rudi> RudiPtr;
typedef QSharedPointer<Rudi> RudiQPtr;

struct Gerd: public Volker
{
    Gerd() { who = QLatin1String("Gerd"); }
    /* reimp */ Gerd * clone() const { return new Gerd( *this ); }
};

typedef shared_ptr<Gerd> GerdPtr;
typedef QSharedPointer<Gerd> GerdQPtr;

Q_DECLARE_METATYPE( Volker* )
Q_DECLARE_METATYPE( Rudi* )
Q_DECLARE_METATYPE( Gerd* )

Q_DECLARE_METATYPE( Rudi )
Q_DECLARE_METATYPE( Gerd )

namespace KPIMUtils
{
  template <> struct SuperClass<Rudi> : public SuperClassTrait<Volker>{};
  template <> struct SuperClass<Gerd> : public SuperClassTrait<Volker>{};
}

QTEST_MAIN( ItemHydra )

ItemHydra::ItemHydra()
{
}

void ItemHydra::initTestCase()
{
}

void ItemHydra::testItemValuePayload()
{
    Item f;
    Rudi rudi;
    f.setPayload( rudi );
    QVERIFY( f.hasPayload() );

    Item b;
    Gerd gerd;
    b.setPayload( gerd );
    QVERIFY( b.hasPayload() );

    QCOMPARE( f.payload<Rudi>(), rudi );
    QVERIFY( !(f.payload<Rudi>() == gerd ) );
    QCOMPARE( b.payload<Gerd>(), gerd);
    QVERIFY( !(b.payload<Gerd>() == rudi) );
}

void ItemHydra::testItemPointerPayload()
{
    Item f;
    Rudi *rudi = new Rudi;

    // the below should not compile
    //f.setPayload( rudi );

    // std::auto_ptr is not copyconstructable and assignable, therefore this will fail as well
    //f.setPayload( std::auto_ptr<Rudi>( rudi ) );
    //QVERIFY( f.hasPayload() );
    //QCOMPARE( f.payload< std::auto_ptr<Rudi> >()->who, rudi->who );

    // below doesn't compile, hopefully
    //QCOMPARE( f.payload< Rudi* >()->who, rudi->who );

    delete rudi;
}

void ItemHydra::testItemCopy()
{
    Item f;
    Rudi rudi;
    f.setPayload( rudi );

    Item r = f;
    QCOMPARE( r.payload<Rudi>(), rudi );

    Item s;
    s = f;
    QVERIFY( s.hasPayload() );
    QCOMPARE( s.payload<Rudi>(), rudi );

}

void ItemHydra::testEmptyPayload()
{
    Item i1;
    Item i2;
    i1 = i2; // should not crash

    QVERIFY( !i1.hasPayload() );
    QVERIFY( !i2.hasPayload() );
    QVERIFY( !i1.hasPayload<Rudi>() );
    QVERIFY( !i1.hasPayload<RudiPtr>() );

    bool caughtException = false;
    bool caughtRightException = true;
    try {
      Rudi r = i1.payload<Rudi>();
    } catch ( const Akonadi::PayloadException &e ) {
      qDebug() << e.what();
      caughtException = true;
      caughtRightException = true;
    } catch ( const Akonadi::Exception & e ) {
      qDebug() << "Caught Akonadi exception of type " << typeid(e).name() << ": " << e.what()
               << ", expected type" << typeid(Akonadi::PayloadException).name();
      caughtException = true;
      caughtRightException = false;
    } catch ( const std::exception & e ) {
      qDebug() << "Caught exception of type " << typeid(e).name() << ": " << e.what()
               << ", expected type" << typeid(Akonadi::PayloadException).name();
      caughtException = true;
      caughtRightException = false;
    } catch ( ... ) {
      qDebug() << "Caught unknown exception";
      caughtException = true;
      caughtRightException = false;
    }
    QVERIFY( caughtException );
    QVERIFY( caughtRightException );
}

void ItemHydra::testPointerPayload()
{
    Rudi* r = new Rudi;
    RudiPtr p( r );
    boost::weak_ptr<Rudi> w( p );
    QCOMPARE( p.use_count(), (long)1 );

    {
      Item i1;
      i1.setPayload(p);
      QVERIFY( i1.hasPayload() );
      QCOMPARE( p.use_count(), (long)2 );
      {
        QVERIFY( i1.hasPayload< RudiPtr >() );
        RudiPtr p2 = i1.payload< RudiPtr >();
        QCOMPARE( p.use_count(), (long)3 );
      }

      {
        QVERIFY( i1.hasPayload< VolkerPtr >() );
        VolkerPtr p2 = i1.payload< VolkerPtr >();
        QCOMPARE( p.use_count(), (long)3 );
      }

      QCOMPARE( p.use_count(), (long)2 );
    }
    QCOMPARE( p.use_count(), (long)1 );
    QCOMPARE( w.use_count(), (long)1 );
    p.reset();
    QCOMPARE( w.use_count(), (long)0 );
}

void ItemHydra::testPolymorphicPayload()
{
  VolkerPtr p( new Rudi );

  {
      Item i1;
      i1.setPayload(p);
      QVERIFY( i1.hasPayload() );
      QVERIFY( i1.hasPayload<VolkerPtr>() );
      QVERIFY( i1.hasPayload<RudiPtr>() );
      QVERIFY( !i1.hasPayload<GerdPtr>() );
      QCOMPARE( p.use_count(), (long)2 );
      {
        RudiPtr p2 = boost::dynamic_pointer_cast<Rudi, Volker>( i1.payload< VolkerPtr >() );
        QCOMPARE( p.use_count(), (long)3 );
        QCOMPARE( p2->who, QLatin1String("Rudi") );
      }

      {
        RudiPtr p2 = i1.payload< RudiPtr >();
        QCOMPARE( p.use_count(), (long)3 );
        QCOMPARE( p2->who, QLatin1String("Rudi") );
      }

      bool caughtException = false;
      try {
        GerdPtr p3 = i1.payload<GerdPtr>();
      } catch ( const Akonadi::PayloadException &e ) {
        qDebug() << e.what();
        caughtException = true;
      }
      QVERIFY( caughtException );

      QCOMPARE( p.use_count(), (long)2 );
  }
}

void ItemHydra::testNullPointerPayload()
{
  RudiPtr p( (Rudi*)0 );
  Item i;
  i.setPayload( p );
  QVERIFY( i.hasPayload() );
  QVERIFY( i.hasPayload<RudiPtr>() );
  QVERIFY( i.hasPayload<VolkerPtr>() );
  QVERIFY( i.hasPayload<GerdPtr>() );
  QCOMPARE( i.payload<RudiPtr>().get(), (Rudi*)0 );
  QCOMPARE( i.payload<VolkerPtr>().get(), (Volker*)0 );
}

void ItemHydra::testQSharedPointerPayload()
{
  RudiQPtr p( new Rudi );
  Item i;
  i.setPayload( p );
  QVERIFY( i.hasPayload() );
  QVERIFY( i.hasPayload<VolkerQPtr>() );
  QVERIFY( i.hasPayload<RudiQPtr>() );
  QVERIFY( !i.hasPayload<GerdQPtr>() );

  {
    VolkerQPtr p2 = i.payload< VolkerQPtr >();
    QCOMPARE( p2->who, QLatin1String("Rudi") );
  }

  {
    RudiQPtr p2 = i.payload< RudiQPtr >();
    QCOMPARE( p2->who, QLatin1String("Rudi") );
  }

  bool caughtException = false;
  try {
    GerdQPtr p3 = i.payload<GerdQPtr>();
  } catch ( const Akonadi::PayloadException &e ) {
    qDebug() << e.what();
    caughtException = true;
  }
  QVERIFY( caughtException );
}

void ItemHydra::testHasPayload()
{
  Item i1;
  QVERIFY( !i1.hasPayload<Rudi>() );
  QVERIFY( !i1.hasPayload<Gerd>() );

  Rudi r;
  i1.setPayload( r );
  QVERIFY( i1.hasPayload<Rudi>() );
  QVERIFY( !i1.hasPayload<Gerd>() );
}

void ItemHydra::testSharedPointerConversions()
{

    Item a;
    RudiQPtr rudi( new Rudi );
    a.setPayload( rudi );
    // only the root base classes should show up with their metatype ids:
    QVERIFY(  a.availablePayloadMetaTypeIds().contains( qMetaTypeId<Volker*>() ) );
    QVERIFY(  a.hasPayload<RudiQPtr>() );
    QVERIFY(  a.hasPayload<VolkerPtr>() );
    QVERIFY(  a.hasPayload<RudiPtr>() );
    QVERIFY( !a.hasPayload<GerdPtr>() );
    QVERIFY(  a.payload<RudiPtr>().get() );
    QVERIFY(  a.payload<VolkerPtr>().get() );
    bool thrown = false, thrownCorrectly = true;
    try {
        QVERIFY( !a.payload<GerdPtr>() );
    } catch ( const Akonadi::PayloadException & e ) {
        thrown = thrownCorrectly = true;
    } catch ( ... ) {
        thrown = true;
        thrownCorrectly = false;
    }
    QVERIFY( thrown );
    QVERIFY( thrownCorrectly );

}

