
#include "public_etm.h"

PublicETM::PublicETM( ChangeRecorder *monitor, QObject *parent )
  : EntityTreeModel( monitor, new PublicETMPrivate( this ), parent )
{
}

PublicETMPrivate::PublicETMPrivate( PublicETM *p )
  : EntityTreeModelPrivate( p )
{
}

