/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dao.h"
#include <QtTest>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>

Akonadi::Collection::List  DAO::showCollections(){
  Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(
                                        Akonadi::Collection::root(), 
                                        Akonadi::CollectionFetchJob::Recursive);

    job->exec();     
    Akonadi::Collection::List collections = job->collections();
    return collections;
}

Akonadi::Collection  DAO::getCollectionByName(const QString &colname){
  Akonadi::Collection::List colist = showCollections();
  
  foreach(const Akonadi::Collection &col, colist){
    if( colname == col.name()  ) return col;  
  }
} 

bool DAO::insertItem(Akonadi::Item item, Akonadi::Collection collection){
  Akonadi::ItemCreateJob *ijob = new Akonadi::ItemCreateJob(item, collection);
  
  return ijob->exec();
}
