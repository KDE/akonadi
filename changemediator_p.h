#ifndef AKONADI_CHANGEMEDIATOR_P_H
#define AKONADI_CHANGEMEDIATOR_P_H

#include <QtCore/QList>
#include <QtCore/QObject>

namespace Akonadi {

class Collection;
class Item;

class ChangeMediator : public QObject
{
  Q_OBJECT
public:
  static ChangeMediator* instance();

  static void registerMonitor( QObject *monitor );
  static void unregisterMonitor( QObject *monitor );

  static void invalidateCollection( const Akonadi::Collection &collection );
  static void invalidateItem( const Akonadi::Item &item );

private Q_SLOTS:
  void do_registerMonitor( QObject *monitor );
  void do_unregisterMonitor( QObject *monitor );

  void do_invalidateCollection( const Akonadi::Collection &collection );
  void do_invalidateItem( const Akonadi::Item &item );

private:
  QList<QObject*> m_monitors;
};

}

#endif
