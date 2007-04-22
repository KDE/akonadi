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
#include <libakonadi/item.h>
#include <qtest_kde.h>
#include <QDebug>

using namespace Akonadi;

struct Volker
{
    bool operator==( const Volker& f ) const
    {
        return f.who == who;
    }
    QString who;
};

struct Rudi: public Volker
{
    Rudi() { who = "Rudi"; }
};

struct Gerd: public Volker
{
    Gerd() { who = "Gerd"; }
};


QTEST_KDEMAIN( ItemHydra, NoGUI )

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

    // the below should assert
    //f.setPayload( rudi );

    f.setPayload( std::auto_ptr<Rudi>( rudi ) );
    QVERIFY( f.hasPayload() );
    QCOMPARE( f.payload< std::auto_ptr<Rudi> >()->who, rudi->who );

    // below doesn't compile, hopefully
    //QCOMPARE( f.payload< Rudi* >()->who, rudi->who );
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
}


#include "itemhydratest.moc"
