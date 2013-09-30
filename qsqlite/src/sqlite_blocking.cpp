#include "sqlite_blocking.h"

#include <sqlite3.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#define usleep(x) Sleep(x/1000)
#endif

#include "qdebug.h"
#include "qstringbuilder.h"
#include "qthread.h"

QString debugString()
{
  return QString( QLatin1Literal( "[QSQLITE3: " ) + QString::number( quint64( QThread::currentThreadId() ) ) + QLatin1Literal( "] " ) );
}

int sqlite3_blocking_step( sqlite3_stmt *pStmt )
{
  // NOTE: The example at http://www.sqlite.org/unlock_notify.html says to wait
  //       for SQLITE_LOCK but for some reason I don't understand I get
  //       SQLITE_BUSY.
  int rc = sqlite3_step( pStmt );

  QThread::currentThreadId();
  if ( rc == SQLITE_BUSY ) {
    qDebug() << debugString() << "sqlite3_blocking_step: Entering while loop";
  }

  while ( rc == SQLITE_BUSY ) {
    usleep( 5000 );
    sqlite3_reset( pStmt );
    rc = sqlite3_step( pStmt );

    if ( rc != SQLITE_BUSY ) {
      qDebug() << debugString() << "sqlite3_blocking_step: Leaving while loop";
    }
  }

  return rc;
}

int sqlite3_blocking_prepare16_v2( sqlite3 *db,           /* Database handle. */
                                   const void *zSql,      /* SQL statement, UTF-16 encoded */
                                   int nSql,              /* Length of zSql in bytes. */
                                   sqlite3_stmt **ppStmt, /* OUT: A pointer to the prepared statement */
                                   const void **pzTail    /* OUT: Pointer to unused portion of zSql */ )
{
  int rc = sqlite3_prepare16_v2( db, zSql, nSql, ppStmt, pzTail );

  if ( rc == SQLITE_BUSY ) {
    qDebug() << debugString() << "sqlite3_blocking_prepare16_v2: Entering while loop";
  }

  while ( rc == SQLITE_BUSY ) {
    usleep( 500000 );
    rc = sqlite3_prepare16_v2( db, zSql, nSql, ppStmt, pzTail );

    if ( rc != SQLITE_BUSY ) {
      qDebug() << debugString() << "sqlite3_prepare16_v2: Leaving while loop";
    }
  }

  return rc;
}
