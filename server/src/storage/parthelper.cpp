/***************************************************************************
 *   Copyright (C) 2009 by Andras Mantia <amantia@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "parthelper.h"
#include "entities.h"
#include "selectquerybuilder.h"
#include "../../libs/xdgbasedirs_p.h"

#include <QDir>
#include <QFile>
#include <QDebug>

//set to true in order to store the payload data in a file
#define AKONADI_USE_FILE_DATA false

using namespace Akonadi;

PartHelper::PartHelper()
{
}


PartHelper::~PartHelper()
{
}

bool PartHelper::update( Part *part )
{
  if (!part)
    return false;

  if (AKONADI_USE_FILE_DATA)
  {
    QString fileName = PartHelper::fileNameForId( part->id() );

    QFile file(fileName);

    if (file.open( QIODevice::WriteOnly | QIODevice::Truncate ))
    {
      QByteArray data = part->data();
      //qDebug() << "Update part file " << part->id() << "_data with " << QString::fromUtf8(data).left(50);

      file.write( data );
      QByteArray fileNameData = fileName.toLocal8Bit();
      part->setData( fileNameData );
      part->setDatasize( fileNameData.size() );
      part->setExternal (true );
      file.close();
    } else
    {
      return false;
    }
  }
  return part->update();
}

bool PartHelper::insert( Part *part, qint64* insertId )
{
  if (!part)
    return false;

  //qDebug() << "Insert original data " << part->data();
  QByteArray fileNameData("");
  QByteArray data;

  //it is needed to insert first the metadata so a new id is generated for the part,
  //and we need this id for the payload file name
  if (AKONADI_USE_FILE_DATA)
  {
    data = part->data();
    part->setData( fileNameData );
    part->setDatasize( fileNameData.size() );
    part->setExternal(true);

  }

  bool result = part->insert( insertId );

  if (AKONADI_USE_FILE_DATA && result)
  {
    QString fileName = PartHelper::fileNameForId( part->id() );

    QFile file( fileName );

    if (file.open( QIODevice::WriteOnly | QIODevice::Truncate ))
    {
      //qDebug() << "Insert part file " << part->id() << "_data with " << QString::fromUtf8(data).left(50);

      file.write(data);
      fileNameData = fileName.toLocal8Bit();
      part->setData(fileNameData);
      part->setDatasize(fileNameData.size());
      part->update();
      file.close();
    } else
    {
      return false;
    }
  }
  return result;
}

bool PartHelper::remove( Akonadi::Part *part )
{
  if (!part)
    return false;

  if (AKONADI_USE_FILE_DATA && part->external())
  {
    //qDebug() << "remove part file " << part->id();
    QString fileName = PartHelper::fileNameForId( part->id() );
    QFile file( fileName );
    file.remove();
  }
  return part->remove();
}

bool PartHelper::remove(qint64 id)
{
  if (AKONADI_USE_FILE_DATA)
  {
    //qDebug() << "remove part file " <<id;
    QString fileName = PartHelper::fileNameForId( id );
    QFile file( fileName );
    file.remove();
  }
  return Part::remove(id);
}

bool PartHelper::remove( const QString &column, const QVariant &value )
{
  if (AKONADI_USE_FILE_DATA)
  {
    SelectQueryBuilder<Part> builder;
    builder.addValueCondition( column, Query::Equals, value );
    if ( !builder.exec() ) {
      qDebug() << "Error selecting records to be deleted from table"
          << Part::tableName() << builder.query().lastError().text();
      return false;
    }
    Part::List parts = builder.result();
    Part::List::Iterator it = parts.begin();
    Part::List::Iterator end = parts.end();
    for ( ; it != end; ++it )
    {
    //qDebug() << "remove part file " <<value.toString();
      QString fileName = PartHelper::fileNameForId( (*it).id() );
      QFile file( fileName );
      file.remove();
    }
  }
  return Part::remove( column, value );
}

void PartHelper::loadData( Part::List &parts )
{
  if (AKONADI_USE_FILE_DATA)
  {
    Part::List::Iterator it = parts.begin();
    Part::List::Iterator end = parts.end();
    for ( ; it != end; ++it )
    {
      loadData( (*it) );
    }
  }
}

void PartHelper::loadData( Part &part )
{
  if (AKONADI_USE_FILE_DATA)
  {
    if (part.external())
    {
      QString fileName = PartHelper::fileNameForId( part.id() );
      //qDebug() << "loadData " << fileName;
      QFile file( fileName );
      if (file.open( QIODevice::ReadOnly ))
      {
        QByteArray data = file.readAll();
        part.setData(data);
        part.setDatasize( data.size() );
        //qDebug() << "load part file " << part.id() << QString::fromUtf8(data).left(50);
        file.close();
      }
    }
  }
}

QByteArray PartHelper::translateData( qint64 id, const QByteArray &data )
{
  if (AKONADI_USE_FILE_DATA)
  {
    //qDebug() << "translateData " << id;
    QString fileName = PartHelper::fileNameForId( id );
    QFile file( fileName );
    if (file.open( QIODevice::ReadOnly ))
    {
      QByteArray payload = file.readAll();
      file.close();
      return payload;
    } else
      return data;
  } else
    return data;
}

/** Returns the record with id @p id. */
Part PartHelper::retrieveById( qint64 id )
{
  Part part = Part::retrieveById( id );
  loadData(part);
  return part;
}

/** Returns the record with name @p name. */
Part PartHelper::retrieveByName( const QString &name )
{
  Part part = Part::retrieveByName( name );
  loadData(part);
  return part;
}

QString PartHelper::fileNameForId( qint64 id )
{
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) ) + QDir::separator();
  return dataDir + QString::number(id);
}
