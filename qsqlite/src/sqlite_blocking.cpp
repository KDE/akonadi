#include "sqlite_blocking.h"

#include <cassert>
#include <pthread.h>
#include <sqlite3.h>

typedef struct UnlockNotification UnlockNotification;
struct UnlockNotification {
  int fired;                           /* True after unlock event has occured */
  pthread_cond_t cond;                 /* Condition variable to wait on */
  pthread_mutex_t mutex;               /* Mutex to protect structure */
};

static void unlock_notify_cb(void **apArg, int nArg) {
  int i;
  for(i=0; i<nArg; i++){
    UnlockNotification *p = (UnlockNotification *) apArg[i];
    pthread_mutex_lock( &p->mutex );
    p->fired = 1;
    pthread_cond_signal( &p->cond );
    pthread_mutex_unlock( &p->mutex );
  }
}

static int wait_for_unlock_notify( sqlite3 *db )
{
  int rc;
  UnlockNotification un;

  /* Initialize the UnlockNotification structure. */
  un.fired = 0;
  pthread_mutex_init( &un.mutex, 0 );
  pthread_cond_init( &un.cond, 0 );

  /* Register for an unlock-notify callback. */
  rc = sqlite3_unlock_notify( db, unlock_notify_cb, (void *) &un );
  assert( rc == SQLITE_LOCKED || rc == SQLITE_OK );

  /* The call to sqlite3_unlock_notify() always returns either SQLITE_LOCKED
  ** or SQLITE_OK.
  **
  ** If SQLITE_LOCKED was returned, then the system is deadlocked. In this
  ** case this function needs to return SQLITE_LOCKED to the caller so
  ** that the current transaction can be rolled back. Otherwise, block
  ** until the unlock-notify callback is invoked, then return SQLITE_OK.
  */
  if( rc == SQLITE_OK ) {
    pthread_mutex_lock( &un.mutex );
    if( !un.fired ) {
      pthread_cond_wait( &un.cond, &un.mutex);
    }
    pthread_mutex_unlock( &un.mutex );
  }

  /* Destroy the mutex and condition variables. */
  pthread_cond_destroy( &un.cond );
  pthread_mutex_destroy( &un.mutex );

  return rc;
}

int sqlite3_blocking_step( sqlite3_stmt *pStmt )
{
  int rc;
  // NOTE: The example at http://www.sqlite.org/unlock_notify.html says to wait
  //       for SQLITE_LOCK but for some reason I don't understand I get
  //       SQLITE_BUSY.
  while( SQLITE_BUSY == ( rc = sqlite3_step( pStmt ) ) ) {
    rc = wait_for_unlock_notify( sqlite3_db_handle( pStmt ) );
    if( rc != SQLITE_OK ) break;
    sqlite3_reset( pStmt );
  }

  return rc;
}

int sqlite3_blocking_prepare16_v2( sqlite3 *db,           /* Database handle. */
                                   const void *zSql,      /* SQL statement, UTF-16 encoded */
                                   int nSql,              /* Length of zSql in bytes. */
                                   sqlite3_stmt **ppStmt, /* OUT: A pointer to the prepared statement */
                                   const void **pzTail    /* OUT: Pointer to unused portion of zSql */ )
{
  int rc;

  while( SQLITE_BUSY == ( rc = sqlite3_prepare16_v2( db, zSql, nSql, ppStmt, pzTail ) ) ) {
    rc = wait_for_unlock_notify( db );
    if( rc != SQLITE_OK ) break;
  }

  return rc;
}
