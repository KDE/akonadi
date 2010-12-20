#include "sqlite_blocking.h"

#include <sqlite3.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "qdebug.h"
#include "qthread.h"

QString debugString()
{
  return QString( "[QSQLITE3: " + QString::number( quint64( QThread::currentThreadId() ) ) + "] " );
}

int sqlite3_blocking_step( sqlite3_stmt *pStmt )
{
  // NOTE: The example at http://www.sqlite.org/unlock_notify.html says to wait
  //       for SQLITE_LOCK but for some reason I don't understand I get
  //       SQLITE_BUSY.
  int rc = sqlite3_step( pStmt );

  QThread::currentThreadId();
  if ( rc == SQLITE_BUSY )
    qDebug() << debugString() + "sqlite3_blocking_step: Entering while loop";

  while( rc == SQLITE_BUSY ) {
#ifdef _WIN32
    Sleep(5);
#else
    usleep(5000);
#endif
    sqlite3_reset( pStmt );
    rc = sqlite3_step( pStmt );

    if ( rc != SQLITE_BUSY ) {
      qDebug() << debugString() + "sqlite3_blocking_step: Leaving while loop";
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

  if ( rc == SQLITE_BUSY )
    qDebug() << debugString() + "sqlite3_blocking_prepare16_v2: Entering while loop";

  while( rc == SQLITE_BUSY ) {
#ifdef _WIN32
    Sleep(500);
#else
    usleep(500000);
#endif
    rc = sqlite3_prepare16_v2( db, zSql, nSql, ppStmt, pzTail );

    if ( rc != SQLITE_BUSY ) {
      qDebug() << debugString() + "sqlite3_prepare16_v2: Leaving while loop";
    }
  }

  return rc;
}

