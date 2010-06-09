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
#include <libs/xdgbasedirs_p.h>

#include <QDir>
#include <QFile>
#include <QDebug>

using namespace Akonadi;

void PartHelper::update( Part *part, const QByteArray &data, qint64 dataSize )
{
  if (!part)
    throw PartHelperException( "Invalid part" );

  QString origFileName;
  // currently external, so recover the filename to delete it after the update succeeded
  if ( part->external() )
    origFileName = QString::fromUtf8( part->data() );

  const bool storeExternal = DbConfig::configuredDatabase()->useExternalPayloadFile() && dataSize > DbConfig::configuredDatabase()->sizeThreshold();

  if ( storeExternal ) {
    QString fileName = origFileName;
    if ( fileName.isEmpty() )
      fileName = fileNameForId( part->pimItemId() );
    QString rev = QString::fromAscii("_r0");
    if ( fileName.contains( QString::fromAscii("_r") ) ) {
      int revIndex = fileName.indexOf(QString::fromAscii("_r"));
      rev = fileName.mid( revIndex + 2  );
      int r = rev.toInt();
      r++;
      rev = QString::number( r );
      fileName = fileName.left( revIndex );
      rev.prepend( QString::fromAscii("_r") );
    }
    fileName += rev;

    QFile file( fileName );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
      if ( file.write( data ) == data.size() ) {
        part->setData( fileName.toLocal8Bit() );
        part->setExternal( true );
      } else {
        throw PartHelperException( QString::fromLatin1( "Failed to write into '%1', error was '%2'" ).arg( fileName ).arg( file.errorString() ) );
      }
      file.close();
    } else  {
     throw PartHelperException( QString::fromLatin1( "Could not open '%1' for writing, error was '%2'" ).arg( fileName ).arg( file.errorString() ) );
    }

  // internal storage
  } else {
    part->setData( data );
    part->setExternal( false );
  }

  part->setDatasize( dataSize );
  const bool result = part->update();
  if ( !result )
    throw PartHelperException( "Failed to update database record" );
  // everything worked, remove the old file
  if ( !origFileName.isEmpty() )
    QFile::remove( origFileName );
}

bool PartHelper::insert( Part *part, qint64* insertId )
{
  if (!part)
    return false;

//   qDebug() << "Insert original data " << part->data();
  QByteArray fileNameData("");
  QByteArray data;
  bool storeInFile = DbConfig::configuredDatabase()->useExternalPayloadFile()  && ( part->datasize() > DbConfig::configuredDatabase()->sizeThreshold() || part->external() );

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
//       qDebug() << "Insert: create part file " << fileName << "with " << QString::fromUtf8(data).left(50);

      file.write(data);
      fileNameData = fileName.toLocal8Bit();
      part->setData(fileNameData);
      part->setDatasize(fileNameData.size());
      part->update();
      file.close();
    } else
    {
//       qDebug() << "Insert: payload file " << fileName << " could not be open for writing!";
//       qDebug() << "Error: " << file.errorString();
      return false;
    }
  }
  return result;
}

bool PartHelper::remove( Akonadi::Part *part )
{
  if (!part)
    return false;

  if ( part->external() ) {
    // qDebug() << "remove part file " << part->data();
    const QString fileName = QString::fromUtf8( part->data() );
    QFile::remove( fileName );
  }
  return part->remove();
}

bool PartHelper::remove( const QString &column, const QVariant &value )
{
  SelectQueryBuilder<Part> builder;
  builder.addValueCondition( column, Query::Equals, value );
  builder.addValueCondition( Part::externalColumn(), Query::Equals, true );
  if ( !builder.exec() ) {
//      qDebug() << "Error selecting records to be deleted from table"
//          << Part::tableName() << builder.query().lastError().text();
    return false;
  }
  const Part::List parts = builder.result();
  Part::List::ConstIterator it = parts.constBegin();
  Part::List::ConstIterator end = parts.constEnd();
  for ( ; it != end; ++it )
  {
    const QString fileName = QString::fromUtf8( (*it).data() );
    // qDebug() << "remove part file " << fileName;
    QFile::remove( fileName );
  }
  return Part::remove( column, value );
}

QByteArray PartHelper::translateData( const QByteArray &data, bool isExternal )
{
  if ( isExternal ) {
    const QString fileName = QString::fromUtf8( data );
    QFile file( fileName );
    if ( file.open( QIODevice::ReadOnly ) ) {
      const QByteArray payload = file.readAll();
      file.close();
      return payload;
    } else {
      qDebug() << "Payload file " << fileName << " could not be open for reading!";
      qDebug() << "Error: " << file.errorString();
      return QByteArray();
    }
  } else {
    // not external
    return data;
  }
}

QByteArray PartHelper::translateData( const Part& part )
{
  return translateData( part.data(), part.external() );
}

QString PartHelper::fileNameForId( qint64 id )
{
  Q_ASSERT( id >= 0 );
  const QString dataDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/file_db_data" ) ) + QDir::separator();
  Q_ASSERT( dataDir != QDir::separator() );
  return dataDir + QString::number(id);
}

bool Akonadi::PartHelper::truncate(Part& part)
{
  if ( part.external() ) {
    const QString fileName = QString::fromUtf8( part.data() );
    QFile::remove( fileName );
  } 

  part.setData( QByteArray() );
  part.setDatasize( 0 );
  return part.update();
}
