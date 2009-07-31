#include "sqlite_blocking.h"

#include <sqlite3.h>
#include <unistd.h>

#include "qdebug.h"

int sqlite3_blocking_step( sqlite3_stmt *pStmt )
{
  // NOTE: The example at http://www.sqlite.org/unlock_notify.html says to wait
  //       for SQLITE_LOCK but for some reason I don't understand I get
  //       SQLITE_BUSY.
  int rc = sqlite3_step( pStmt );

  if ( rc == SQLITE_BUSY )
    qDebug() << "[QSQLITE3] STEP: Entering while loop";

  while( rc == SQLITE_BUSY ) {
    usleep(500000);
    sqlite3_reset( pStmt );
    rc = sqlite3_step( pStmt );
  }

  qDebug() << "[QSQLITE3] BLOCKIN_STEP Returning" << rc;
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
    qDebug() << "[QSQLITE3] PREPARE: Entering while loop";

  while( rc == SQLITE_BUSY ) {
    usleep(500000);
    rc = sqlite3_prepare16_v2( db, zSql, nSql, ppStmt, pzTail );
  }

  qDebug() << "[QSQLITE3] BLOCKIN_PREPARE Returning" << rc;
  return rc;
}
