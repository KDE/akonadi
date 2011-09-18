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

  public Q_SLOTS:
    void registerMonitor( QObject *monitor );
    void unregisterMonitor( QObject *monitor );

    void invalidateCollection( const Akonadi::Collection &collection );
    void invalidateItem( const Akonadi::Item &item );

  private:
    QList<QObject*> m_monitors;
};

}

#endif
