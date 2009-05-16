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
#include "dbconfig.h"
#include "../../libs/xdgbasedirs_p.h"

#include <QDir>
#include <QFile>
#include <QDebug>

using namespace Akonadi;

PartHelper::PartHelper()
{
}


PartHelper::~PartHelper()
{
}

bool PartHelper::update( Part *part, const QByteArray &data, qint64 dataSize )
{
  if (!part)
    return false;

  if (DbConfig::useExternalPayloadFile() && part->external())
  {
    QString origFileName = QString::fromUtf8( part->data() );
    QString fileName = origFileName;
    QString rev = QString::fromAscii("_r0");
    if (fileName.contains( QString::fromAscii("_r") ))
    {
      int revIndex = fileName.indexOf(QString::fromAscii("_r"));
      rev = fileName.mid( revIndex + 2  );
      int r = rev.toInt();
      r++;
      rev = QString::number( r );
      fileName = fileName.left( revIndex );
      rev.prepend( QString::fromAscii("_r") );
    }
    fileName += rev;

    QFile file(fileName);

    if (file.open( QIODevice::WriteOnly | QIODevice::Truncate ))
    {
      qDebug() << "Update part file " << fileName <<" with " << QString::fromUtf8(data).left(50);

      file.write( data );
      QByteArray fileNameData = fileName.toLocal8Bit();
      part->setData( fileNameData );
      part->setDatasize( fileNameData.size() );
      part->setExternal( true );
      file.close();
      qDebug() << "Removing part file " << origFileName;
      QFile::remove(origFileName);
    } else
    {
      qDebug() << "Update: payload file " << fileName << " could not be open for writing!";
      qDebug() << "Error: " << file.errorString();
      return false;
    }
  } else
  {
    part->setData( data );
    part->setDatasize( dataSize );
    part->setExternal( false );
  }
  return part->update();
}

bool PartHelper::insert( Part *part, qint64* insertId )
{
  if (!part)
    return false;

//   qDebug() << "Insert original data " << part->data();
  QByteArray fileNameData("");
  QByteArray data;
  bool storeInFile = DbConfig::useExternalPayloadFile()  && ( part->datasize() > DbConfig::sizeThreshold() || part->external() );

  //it is needed to insert first the metadata so a new id is generated for the part,
  //and we need this id for the payload file name
  if (storeInFile)
  {
    data = part->data();
    part->setData( fileNameData );
    part->setDatasize( fileNameData.size() );
    part->setExternal( true );
  } else
  {
    part->setExternal( false );
  }

  bool result = part->insert( insertId );

  if (storeInFile && result)
  {
    QString fileName = PartHelper::fileNameForId( part->id() );
    fileName +=  QString::fromUtf8("_r0");

    QFile file( fileName );

    if (file.open( QIODevice::WriteOnly | QIODevice::Truncate ))
    {
      qDebug() << "Insert: create part file " << fileName << "with " << QString::fromUtf8(data).left(50);

      file.write(data);
      fileNameData = fileName.toLocal8Bit();
      part->setData(fileNameData);
      part->setDatasize(fileNameData.size());
      part->update();
      file.close();
    } else
    {
      qDebug() << "Insert: payload file " << fileName << " could not be open for writing!";
      qDebug() << "Error: " << file.errorString();
      return false;
    }
  }
  return result;
}

bool PartHelper::remove( Akonadi::Part *part )
{
  if (!part)
    return false;

  if (DbConfig::useExternalPayloadFile()  && part->external())
  {
    qDebug() << "remove part file " << part->data();
    QString fileName = QString::fromUtf8( part->data() );
    QFile::remove( fileName );
  }
  return part->remove();
}

bool PartHelper::remove( const QString &column, const QVariant &value )
{
  if ( DbConfig::useExternalPayloadFile() )
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
      if ((*it).external())
      {
        QString fileName = QString::fromUtf8( (*it).data() );
        qDebug() << "remove part file " << fileName;
        QFile::remove( fileName );
      }
    }
  }
  return Part::remove( column, value );
}

bool PartHelper::loadData( Part::List &parts )
{
  Part::List::Iterator it = parts.begin();
  Part::List::Iterator end = parts.end();
  for ( ; it != end; ++it )
  {
    if ( !loadData( (*it) ) )
    {
      return false;
    }
  }

  return true;
}

bool PartHelper::loadData( Part &part )
{
  if ( DbConfig::useExternalPayloadFile() && part.external() )
  {
    QString fileName = QString::fromUtf8( part.data() );
    QFile file( fileName );
    if (file.open( QIODevice::ReadOnly ))
    {
      QByteArray data = file.readAll();
      part.setData( data );
      part.setDatasize( data.size() );
      qDebug() << "load part file " << fileName << QString::fromUtf8(data).left(50);
      file.close();
    } else
    {
      qDebug() << "Payload file " << fileName << " could not be open for reading!";
      qDebug() << "Error: " << file.errorString();
      return false;
    }
  } else
  if ( part.external() ) //external payload is disabled, but the item is marked as external
  {
    return false;
  }

  return true;
}

QByteArray PartHelper::translateData( qint64 id, const QByteArray &data, bool isExternal )
{
  Q_UNUSED(id);

  if ( DbConfig::useExternalPayloadFile() && isExternal )
  {
    //qDebug() << "translateData " << id;
    QString fileName = QString::fromUtf8( data );
    QFile file( fileName );
    if (file.open( QIODevice::ReadOnly ))
    {
      QByteArray payload = file.readAll();
      file.close();
      return payload;
    } else
    {
      qDebug() << "Payload file " << fileName << " could not be open for reading!";
      qDebug() << "Error: " << file.errorString();
      return QByteArray();
    }
  } else
  if ( isExternal ) //external payload is disabled, but the item is marked as external
  {
    return QByteArray();
  } else
    return data;
}

QByteArray PartHelper::translateData( const Part& part )
{
  return translateData( part.id(), part.data(), part.external() );
}

/** Returns the record with id @p id. */
Part PartHelper::retrieveById( qint64 id )
{
  Part part = Part::retrieveById( id );
  loadData(part);
  return part;
}

QString PartHelper::fileNameForId( qint64 id )
{
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) ) + QDir::separator();
  return dataDir + QString::number(id);
}
