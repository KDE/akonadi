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
#include "dao.h"
#include "item.h"
#include "itemfactory.h"
#include "config.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>

AkonadiTesting::AkonadiTesting( const QString &configFileName )
{
  Q_UNUSED( configFileName ) // TODO use this if !isNull()
}

AkonadiTesting::AkonadiTesting()
{
}

AkonadiTesting::~AkonadiTesting()
{
}

void AkonadiTesting::insertItem( const QString &fileName, const QString &collectionName )
{
  ItemFactory factory;
  DAO dao;

  const Akonadi::Collection collection = dao.collectionByName( collectionName );

  const Item *item = factory.createItem( fileName );

  foreach ( const Akonadi::Item &akonadItem, item->items() ) {
    if ( dao.insertItem( akonadItem, collection ) ) {
      qDebug()<<"Item loaded to Akonadi";
    } else {
      qDebug()<<"Item can not be loaded";
    }
  }
}

void AkonadiTesting::insertItemFromList()
{
  const Config *config = Config::instance();
  QPair<QString, QString> configItem;

  foreach (configItem, config->itemConfig() ) {
    insertItem( configItem.first, configItem.second );
  }
}
