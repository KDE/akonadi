
#include "inspectablemonitor.h"

FakeNotificationSource::FakeNotificationSource(QObject* parent): QObject(parent)
{

}


InspectableMonitorPrivate::InspectableMonitorPrivate(FakeMonitorDependeciesFactory *dependenciesFactory, InspectableMonitor* parent)
  : Akonadi::MonitorPrivate(dependenciesFactory, parent)
{
}

void InspectableMonitor::doConnectToNotificationManager()
{
  d_ptr->connectToNotificationManager();
}

InspectableMonitor::InspectableMonitor(FakeMonitorDependeciesFactory *dependenciesFactory, QObject *parent)
  : Monitor(new InspectableMonitorPrivate(dependenciesFactory, this), parent)
{
  // Make sure signals don't get optimized away.
  // TODO: Make this parametrizable in the test class.
  connect(this, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemRemoved(Akonadi::Item)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionChanged(Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionRemoved(Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionUnsubscribed(Akonadi::Collection)), SIGNAL(dummySignal()));

  QTimer::singleShot(0, this, SLOT(doConnectToNotificationManager()));
}

#include "inspectablemonitor.moc"
