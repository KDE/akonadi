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

#include "akonaditesting.h"
#include "itemfactory.h"
#include "config.h"
#include <QFile>
#include <akonadi/item.h>
#include <QtTest>
#include <QList>

AkonadiTesting::AkonadiTesting( const QString &configfilename )
{
  Q_UNUSED( configfilename ) // TODO use this if !isNull()
}

AkonadiTesting::AkonadiTesting()
{
}
void AkonadiTesting::insertItem(const QString &filename, const QString &colname)
{
  ItemFactory factory;
  DAO dao;

  Akonadi::Collection collection = dao.getCollectionByName(colname);

  QFile *file = new QFile(filename);
  qDebug()<<file->fileName();
  file->open(QFile::ReadOnly);

  Item *item = factory.createItem(file);

  foreach(const Akonadi::Item &akonaditem, item->getItem()){
    if( dao.insertItem(akonaditem, collection ) ){
      qDebug()<<"Item loaded to Akonadi";
    } else {
      qDebug()<<"Item can not be loaded";
    }
  }

  delete file;
}

void AkonadiTesting::insertItemFromList()
{
  Config *config = Config::getInstance();
  QPair<QString, QString> confitem;

  foreach(confitem, config->getItemConfig() ){
    insertItem(confitem.first, confitem.second);
  }

  //delete config;
}
AkonadiTesting::~AkonadiTesting()
{
}


