#ifndef DAO_H
#define DAO_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/collectionfetchjob.h>
#include <QList>

class DAO{
  
  
public:
  bool insertItem(Akonadi::Item item, Akonadi::Collection collection);
  Akonadi::Collection::List showCollections();
  Akonadi::Collection getCollectionByName(const QString &colname);
};

#endif
