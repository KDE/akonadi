#include <QtTest/QtTest>
#include <QVariant>
#include <QMetaType>

#include "response.h"
#include "handler.h"
#include "handler/list.h"
#include "mockobjects.h"

using namespace Akonadi;

class TestHandler: public QObject {
  Q_OBJECT
private slots:

    void initTestCase()
    {
        qRegisterMetaType<Response>("Response");
    }

    void testInit()
    { 
        
    }


    /// ---- List ----

    void testSeparatorList()
    {
        Handler* l = getHandlerFor("LIST");
        QVERIFY( dynamic_cast<List*>(l) != 0 );

        const QByteArray line = "1 LIST \"\" \"\"";

        QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
        l->handleLine( line );
        QCOMPARE(spy.count(), 2);

        const QString expectedFirstResponse = "* LIST (\\Noselect) \"/\" \"\"";
        QVERIFY(nextResponse(spy).asString() == expectedFirstResponse);

        const QString expectedSecondResponse = "1 OK List completed";
        QVERIFY(nextResponse(spy).asString() == expectedSecondResponse);
    }

    void testRootPercentList()
    {
        Handler* l = getHandlerFor("LIST");
        QVERIFY( dynamic_cast<List*>(l) != 0 );

        const QByteArray line = "1 LIST \"\" \"%\"";

        QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
        l->handleLine( line );
        QCOMPARE(spy.count(), 2);

        const QByteArray expectedFirstResponse = "* LIST () \"/\" \"INBOX\"";
        QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

        const QByteArray expectedSecondResponse = "1 OK List completed";
        QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );
    }

    void testRootStarList()
    {
        Handler* l = getHandlerFor("LIST");
        QVERIFY( dynamic_cast<List*>(l) != 0 );

        const QByteArray line = "1 LIST \"\" \"*\"";

        QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
        l->handleLine( line );
        QCOMPARE(spy.count(), 3);

        const QByteArray expectedFirstResponse = "* LIST () \"/\" \"INBOX\"";
        QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

        const QByteArray expectedSecondResponse = "* LIST () \"/\" \"INBOX/foo\"";
        QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );

        const QByteArray expectedThirdResponse = "1 OK List completed";
        QCOMPARE(nextResponse(spy).asString(), expectedThirdResponse );
    }

    void testInboxList()
    {
        Handler* l = getHandlerFor("LIST");
        QVERIFY( dynamic_cast<List*>(l) != 0 );

        const QByteArray line = "1 LIST \"\" \"INBOX\"";

        QSignalSpy spy(l, SIGNAL( responseAvailable( const Response& )));
        l->handleLine( line );
        QCOMPARE(spy.count(), 3);

        const QByteArray expectedFirstResponse = "* LIST () \"/\" \"foo\"";
        QCOMPARE(nextResponse(spy).asString(), expectedFirstResponse );

        const QByteArray expectedSecondResponse = "* LIST () \"/\" \"bar\"";
        QCOMPARE(nextResponse(spy).asString(), expectedSecondResponse );

        const QByteArray expectedThirdResponse = "1 OK List completed";
        QCOMPARE(nextResponse(spy).asString(), expectedThirdResponse );
    }
    
private:
    // Helper
    Response nextResponse( QSignalSpy& spy )
    {
        QList<QVariant> arguments = spy.takeFirst();
        Response r = qvariant_cast<Response>(arguments.at(0));
        //qDebug() << "Response: " << r.asString();
        return r;
    }

    Handler* getHandlerFor(const QByteArray& command )
    {
        Handler *h = Handler::findHandlerForCommandAuthenticated( command );
        if( h != 0 ) {
            h->setTag("1");
            h->setConnection( MockObjects::mockConnection() );
        }
        return h;
    }


};
Q_DECLARE_METATYPE(Response)
QTEST_MAIN(TestHandler)

#include "main.moc"
