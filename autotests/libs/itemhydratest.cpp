/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemhydratest.h"

#include <memory>
#include "item.h"
#include <QTest>
#include <QDebug>
#include <QSharedPointer>
#include <QTemporaryFile>

using namespace Akonadi;

struct Volker {
    bool operator==(const Volker &f) const
    {
        return f.who == who;
    }
    virtual ~Volker() { }
    virtual Volker *clone() const = 0;
    QString who;
};
typedef std::shared_ptr<Volker> VolkerPtr;
typedef QSharedPointer<Volker> VolkerQPtr;

struct Rudi: public Volker {
    Rudi()
    {
        who = QStringLiteral("Rudi");
    }
    virtual ~Rudi() { }
    Rudi *clone() const override
    {
        return new Rudi(*this);
    }
};

typedef std::shared_ptr<Rudi> RudiPtr;
typedef QSharedPointer<Rudi> RudiQPtr;

struct Gerd: public Volker {
    Gerd()
    {
        who = QStringLiteral("Gerd");
    }
    Gerd *clone() const override
    {
        return new Gerd(*this);
    }

    typedef Volker SuperClass;
};

typedef std::shared_ptr<Gerd> GerdPtr;
typedef QSharedPointer<Gerd> GerdQPtr;

Q_DECLARE_METATYPE(Volker *)
Q_DECLARE_METATYPE(Rudi *)
Q_DECLARE_METATYPE(Gerd *)

Q_DECLARE_METATYPE(Rudi)
Q_DECLARE_METATYPE(Gerd)

namespace Akonadi
{
template <> struct SuperClass<Rudi> : public SuperClassTrait<Volker> {};
}

QTEST_MAIN(ItemHydra)

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
    f.setPayload(rudi);
    QVERIFY(f.hasPayload());

    Item b;
    Gerd gerd;
    b.setPayload(gerd);
    QVERIFY(b.hasPayload());

    QCOMPARE(f.payload<Rudi>(), rudi);
    QVERIFY(!(f.payload<Rudi>() == gerd));
    QCOMPARE(b.payload<Gerd>(), gerd);
    QVERIFY(!(b.payload<Gerd>() == rudi));
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
    f.setPayload(rudi);

    Item r = f;
    QCOMPARE(r.payload<Rudi>(), rudi);

    Item s;
    s = f;
    QVERIFY(s.hasPayload());
    QCOMPARE(s.payload<Rudi>(), rudi);

}

void ItemHydra::testEmptyPayload()
{
    Item i1;
    Item i2;
    i1 = i2; // should not crash

    QVERIFY(!i1.hasPayload());
    QVERIFY(!i2.hasPayload());
    QVERIFY(!i1.hasPayload<Rudi>());
    QVERIFY(!i1.hasPayload<RudiPtr>());

    bool caughtException = false;
    bool caughtRightException = true;
    try {
        Rudi r = i1.payload<Rudi>();
    } catch (const Akonadi::PayloadException &e) {
        qDebug() << e.what();
        caughtException = true;
        caughtRightException = true;
    } catch (const Akonadi::Exception &e) {
        qDebug() << "Caught Akonadi exception of type " << typeid(e).name() << ": " << e.what()
                 << ", expected type" << typeid(Akonadi::PayloadException).name();
        caughtException = true;
        caughtRightException = false;
    } catch (const std::exception &e) {
        qDebug() << "Caught exception of type " << typeid(e).name() << ": " << e.what()
                 << ", expected type" << typeid(Akonadi::PayloadException).name();
        caughtException = true;
        caughtRightException = false;
    } catch (...) {
        qDebug() << "Caught unknown exception";
        caughtException = true;
        caughtRightException = false;
    }
    QVERIFY(caughtException);
    QVERIFY(caughtRightException);
}

void ItemHydra::testPointerPayload()
{
    Rudi *r = new Rudi;
    RudiPtr p(r);
    std::weak_ptr<Rudi> w(p);
    QCOMPARE(p.use_count(), (long)1);

    {
        Item i1;
        i1.setPayload(p);
        QVERIFY(i1.hasPayload());
        QCOMPARE(p.use_count(), (long)2);
        {
            QVERIFY(i1.hasPayload< RudiPtr >());
            RudiPtr p2 = i1.payload< RudiPtr >();
            QCOMPARE(p.use_count(), (long)3);
        }

        {
            QVERIFY(i1.hasPayload< VolkerPtr >());
            VolkerPtr p2 = i1.payload< VolkerPtr >();
            QCOMPARE(p.use_count(), (long)3);
        }

        QCOMPARE(p.use_count(), (long)2);
    }
    QCOMPARE(p.use_count(), (long)1);
    QCOMPARE(w.use_count(), (long)1);
    p.reset();
    QCOMPARE(w.use_count(), (long)0);
}

void ItemHydra::testPolymorphicPayloadWithTrait()
{
    VolkerPtr p(new Rudi);

    {
        Item i1;
        i1.setPayload(p);
        QVERIFY(i1.hasPayload());
        QVERIFY(i1.hasPayload<VolkerPtr>());
        QVERIFY(i1.hasPayload<RudiPtr>());
        QVERIFY(!i1.hasPayload<GerdPtr>());
        QCOMPARE(p.use_count(), (long)2);
        {
            RudiPtr p2 = std::dynamic_pointer_cast<Rudi, Volker>(i1.payload< VolkerPtr >());
            QCOMPARE(p.use_count(), (long)3);
            QCOMPARE(p2->who, QStringLiteral("Rudi"));
        }

        {
            RudiPtr p2 = i1.payload< RudiPtr >();
            QCOMPARE(p.use_count(), (long)3);
            QCOMPARE(p2->who, QStringLiteral("Rudi"));
        }

        bool caughtException = false;
        try {
            GerdPtr p3 = i1.payload<GerdPtr>();
        } catch (const Akonadi::PayloadException &e) {
            qDebug() << e.what();
            caughtException = true;
        }
        QVERIFY(caughtException);

        QCOMPARE(p.use_count(), (long)2);
    }
}

void ItemHydra::testPolymorphicPayloadWithTypedef()
{
    VolkerPtr p(new Gerd);

    {
        Item i1;
        i1.setPayload(p);
        QVERIFY(i1.hasPayload());
        QVERIFY(i1.hasPayload<VolkerPtr>());
        QVERIFY(!i1.hasPayload<RudiPtr>());
        QVERIFY(i1.hasPayload<GerdPtr>());
        QCOMPARE(p.use_count(), (long)2);
        {
            auto p2 = std::dynamic_pointer_cast<Gerd, Volker>(i1.payload< VolkerPtr >());
            QCOMPARE(p.use_count(), (long)3);
            QCOMPARE(p2->who, QStringLiteral("Gerd"));
        }

        {
            auto p2 = i1.payload< GerdPtr >();
            QCOMPARE(p.use_count(), (long)3);
            QCOMPARE(p2->who, QStringLiteral("Gerd"));
        }

        bool caughtException = false;
        try {
            auto p3 = i1.payload<RudiPtr>();
        } catch (const Akonadi::PayloadException &e) {
            qDebug() << e.what();
            caughtException = true;
        }
        QVERIFY(caughtException);

        QCOMPARE(p.use_count(), (long)2);
    }
}

void ItemHydra::testNullPointerPayload()
{
    RudiPtr p(static_cast<Rudi *>(nullptr));
    Item i;
    i.setPayload(p);
    QVERIFY(i.hasPayload());
    QVERIFY(i.hasPayload<RudiPtr>());
    QVERIFY(i.hasPayload<VolkerPtr>());
    // Fails, because GerdQPtr is QSharedPointer, while RudiPtr is std::shared_ptr
    // and we cannot do sharedptr casting for null pointers
    QVERIFY(!i.hasPayload<GerdQPtr>());
    QCOMPARE(i.payload<RudiPtr>().get(), (Rudi *)nullptr);
    QCOMPARE(i.payload<VolkerPtr>().get(), (Volker *)nullptr);
}

void ItemHydra::testQSharedPointerPayload()
{
    RudiQPtr p(new Rudi);
    Item i;
    i.setPayload(p);
    QVERIFY(i.hasPayload());
    QVERIFY(i.hasPayload<VolkerQPtr>());
    QVERIFY(i.hasPayload<RudiQPtr>());
    QVERIFY(!i.hasPayload<GerdQPtr>());

    {
        VolkerQPtr p2 = i.payload< VolkerQPtr >();
        QCOMPARE(p2->who, QStringLiteral("Rudi"));
    }

    {
        RudiQPtr p2 = i.payload< RudiQPtr >();
        QCOMPARE(p2->who, QStringLiteral("Rudi"));
    }

    bool caughtException = false;
    try {
        GerdQPtr p3 = i.payload<GerdQPtr>();
    } catch (const Akonadi::PayloadException &e) {
        qDebug() << e.what();
        caughtException = true;
    }
    QVERIFY(caughtException);
}

void ItemHydra::testHasPayload()
{
    Item i1;
    QVERIFY(!i1.hasPayload<Rudi>());
    QVERIFY(!i1.hasPayload<Gerd>());

    Rudi r;
    i1.setPayload(r);
    QVERIFY(i1.hasPayload<Rudi>());
    QVERIFY(!i1.hasPayload<Gerd>());
}

void ItemHydra::testSharedPointerConversions()
{

    Item a;
    RudiQPtr rudi(new Rudi);
    a.setPayload(rudi);
    // only the root base classes should show up with their metatype ids:
    QVERIFY(a.availablePayloadMetaTypeIds().contains(qMetaTypeId<Volker *>()));
    QVERIFY(a.hasPayload<RudiQPtr>());
    QVERIFY(a.hasPayload<VolkerPtr>());
    QVERIFY(a.hasPayload<RudiPtr>());
    QVERIFY(!a.hasPayload<GerdPtr>());
    QVERIFY(a.payload<RudiPtr>().get());
    QVERIFY(a.payload<VolkerPtr>().get());
    bool thrown = false;

    bool thrownCorrectly = true;
    try {
        QVERIFY(!a.payload<GerdPtr>());
    } catch (const Akonadi::PayloadException &e) {
        thrown = thrownCorrectly = true;
    } catch (...) {
        thrown = true;
        thrownCorrectly = false;
    }
    QVERIFY(thrown);
    QVERIFY(thrownCorrectly);

}

void ItemHydra::testForeignPayload()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("123456789");
    file.close();

    Item a(QStringLiteral("application/octet-stream"));
    a.setPayloadPath(file.fileName());
    QVERIFY(a.hasPayload<QByteArray>());
    QCOMPARE(a.payload<QByteArray>(), QByteArray("123456789"));

    Item b(QStringLiteral("application/octet-stream"));
    b.apply(a);
    QVERIFY(b.hasPayload<QByteArray>());
    QCOMPARE(b.payload<QByteArray>(), QByteArray("123456789"));
    QCOMPARE(b.payloadPath(), file.fileName());

    Item c = b;
    QVERIFY(c.hasPayload<QByteArray>());
    QCOMPARE(c.payload<QByteArray>(), QByteArray("123456789"));
    QCOMPARE(c.payloadPath(), file.fileName());
}
