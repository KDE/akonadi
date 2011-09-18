#include "changemediator_p.h"

#include "collection.h"
#include "item.h"

#include <kglobal.h>

using namespace Akonadi;

K_GLOBAL_STATIC(ChangeMediator, s_globalChangeMediator)

ChangeMediator* ChangeMediator::instance()
{
  if (s_globalChangeMediator.isDestroyed())
    return 0;
  else
    return s_globalChangeMediator;
}

void ChangeMediator::registerMonitor(QObject *monitor)
{
  QMetaObject::invokeMethod(instance(), "do_registerMonitor", Q_ARG(QObject* ,monitor));
}

void ChangeMediator::unregisterMonitor(QObject *monitor)
{
  QMetaObject::invokeMethod(instance(), "do_unregisterMonitor", Q_ARG(QObject*, monitor));
}

void ChangeMediator::invalidateCollection(const Akonadi::Collection& collection)
{
  QMetaObject::invokeMethod(instance(), "do_invalidateCollection", Q_ARG(Akonadi::Collection, collection));
}

void ChangeMediator::invalidateItem(const Akonadi::Item& item)
{
  QMetaObject::invokeMethod(instance(), "do_invalidateItem", Q_ARG(Akonadi::Item, item));
}

void ChangeMediator::do_registerMonitor( QObject *monitor )
{
  m_monitors.append( monitor );
}

void ChangeMediator::do_unregisterMonitor( QObject *monitor )
{
  m_monitors.removeAll( monitor );
}

void ChangeMediator::do_invalidateCollection( const Akonadi::Collection &collection )
{
  foreach ( QObject *monitor, m_monitors )
    QMetaObject::invokeMethod( monitor, "invalidateCollectionCache", Qt::AutoConnection, Q_ARG( qint64, collection.id() ) );
}

void ChangeMediator::do_invalidateItem( const Akonadi::Item &item )
{
  foreach ( QObject *monitor, m_monitors )
    QMetaObject::invokeMethod( monitor, "invalidateItemCache", Qt::AutoConnection, Q_ARG( qint64, item.id() ) );
}

#include "changemediator_p.moc"
