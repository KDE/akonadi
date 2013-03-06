commit 8b832c8430c53f39996749804155db8a863e0da2
Author: Wolfgang Rohdewald <wolfgang@rohdewald.de>
Date:   Tue Mar 5 20:20:24 2013 +0100

    fix warning if nepomuk not available
    
    and improve some warning/error messages

diff --git a/server/src/handler/akappend.cpp b/server/src/handler/akappend.cpp
index 4253c42..ef0cedb 100644
--- a/server/src/handler/akappend.cpp
+++ b/server/src/handler/akappend.cpp
@@ -55,7 +55,7 @@ bool Akonadi::AkAppend::commit()
 
     Collection col = HandlerHelper::collectionFromIdOrName( m_mailbox );
     if ( !col.isValid() )
-      return failureResponse( "Unknown collection." );
+      return failureResponse( QByteArray( "Unknown collection for '" ) + m_mailbox + QByteArray( "'." ) );
 
     QByteArray mt;
     QString remote_id;
diff --git a/server/src/handler/append.cpp b/server/src/handler/append.cpp
index 2928fa4..ba00d5a 100644
--- a/server/src/handler/append.cpp
+++ b/server/src/handler/append.cpp
@@ -91,7 +91,7 @@ bool Append::commit()
 
     Collection col = HandlerHelper::collectionFromIdOrName( m_mailbox );
     if ( !col.isValid() )
-      return failureResponse( "Unknown collection." );
+      return failureResponse( QByteArray( "Unknown collection for '" ) + m_mailbox + QByteArray( "'." ) );
 
     QByteArray mt;
     QString remote_id;
diff --git a/server/src/nepomuksearch.cpp b/server/src/nepomuksearch.cpp
index caf38a9..e2047c6 100644
--- a/server/src/nepomuksearch.cpp
+++ b/server/src/nepomuksearch.cpp
@@ -49,16 +49,14 @@ NepomukSearch::NepomukSearch( QObject* parent )
 
 NepomukSearch::~NepomukSearch()
 {
-  if ( mSearchService ) {
-    mSearchService->close();
-    delete mSearchService;
-  }
+  mSearchService->close();
+  delete mSearchService;
 }
 
 QStringList NepomukSearch::search( const QString &query )
 {
   //qDebug() << Q_FUNC_INFO << query;
-  if ( !mSearchService ) {
+  if ( !mSearchService->serviceAvailable() ) {
     qWarning() << "Nepomuk search service not available!";
     return QStringList();
   }
@@ -70,7 +68,7 @@ QStringList NepomukSearch::search( const QString &query )
   encodedRps.insert( QString::fromLatin1( "reqProp1" ), QUrl(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId")).toString() );
 
   if ( !mSearchService->blockingQuery( query, encodedRps ) ) {
-    qWarning() << Q_FUNC_INFO << "Calling blockingQuery() failed!";
+    qWarning() << Q_FUNC_INFO << "Calling blockingQuery() failed!" << query;
     return QStringList();
   }
 
@@ -109,7 +107,7 @@ void NepomukSearch::addHit(const Nepomuk::Query::Result& result)
 
 void NepomukSearch::hitsAdded( const QList<Nepomuk::Query::Result>& entries )
 {
-  if ( !mSearchService ) {
+  if ( !mSearchService->serviceAvailable() ) {
     qWarning() << "Nepomuk search service not available!";
     return;
   }
