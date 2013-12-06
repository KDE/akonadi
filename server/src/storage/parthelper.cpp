/***************************************************************************
 *   Copyright (C) 2009 by Andras Mantia <amantia@kde.org>                 *
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>                    *
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
#include <akdebug.h>
#include "entities.h"
#include "selectquerybuilder.h"
#include "dbconfig.h"
#include "parttypehelper.h"
#include "imapstreamparser.h"
#include <akstandarddirs.h>
#include <libs/xdgbasedirs_p.h>
#include <libs/imapparser_p.h>
#include <libs/protocol_p.h>

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QFileInfo>

#include <QSqlError>

using namespace Akonadi;

QString PartHelper::fileNameForPart( Part *part )
{
  Q_ASSERT( part->id() >= 0 );
  return QString::number( part->id() );
}

void PartHelper::update( Part *part, const QByteArray &data, qint64 dataSize )
{
  if ( !part ) {
    throw PartHelperException( "Invalid part" );
  }

  QString origFileName;
  QString origFilePath;

  // currently external, so recover the filename to delete it after the update succeeded
  if ( part->external() ) {
    origFileName = QString::fromUtf8( part->data() );
    QFileInfo fi( origFileName );
    if ( fi.isAbsolute() ) {
      origFilePath = origFileName;
      origFileName = fi.fileName();
    } else if ( !origFileName.isEmpty() ) {
      origFilePath = storagePath() + origFileName;
    }
  }

  const bool storeExternal = dataSize > DbConfig::configuredDatabase()->sizeThreshold();

  if ( storeExternal ) {
    QString fileName = origFileName;
    if ( fileName.isEmpty() ) {
      fileName = fileNameForPart( part );
    }
    QString rev = QString::fromAscii( "_r0" );
    if ( fileName.contains( QString::fromAscii( "_r" ) ) ) {
      int revIndex = fileName.indexOf( QString::fromAscii( "_r" ) );
      rev = fileName.mid( revIndex + 2 );
      int r = rev.toInt();
      r++;
      rev = QString::number( r );
      fileName = fileName.left( revIndex );
      rev.prepend( QString::fromAscii( "_r" ) );
    }
    fileName += rev;

    QFile file( storagePath() + fileName );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
      if ( file.write( data ) == data.size() ) {
        part->setData( fileName.toLocal8Bit() );
        part->setExternal( true );
      } else {
        throw PartHelperException( QString::fromLatin1( "Failed to write into '%1', error was '%2'" ).arg( file.fileName() ).arg( file.errorString() ) );
      }
      file.close();
    } else  {
     throw PartHelperException( QString::fromLatin1( "Could not open '%1' for writing, error was '%2'" ).arg( file.fileName() ).arg( file.errorString() ) );
    }

  // internal storage
  } else {
    part->setData( data );
    part->setExternal( false );
  }

  part->setDatasize( dataSize );
  const bool result = part->update();
  if ( !result ) {
    throw PartHelperException( "Failed to update database record" );
  }
  // everything worked, remove the old file
  if ( !origFilePath.isEmpty() ) {
    removeFile( origFilePath );
  }
}

bool PartHelper::insert( Part *part, qint64 *insertId )
{
  if ( !part ) {
    return false;
  }

  const bool storeInFile = part->datasize() > DbConfig::configuredDatabase()->sizeThreshold();

  //it is needed to insert first the metadata so a new id is generated for the part,
  //and we need this id for the payload file name
  QByteArray data;
  if ( storeInFile ) {
    data = part->data();
    part->setData( QByteArray() );
    part->setExternal( true );
  } else {
    part->setExternal( false );
  }

  bool result = part->insert( insertId );

  if ( storeInFile && result ) {
    QString fileName = fileNameForPart( part );
    fileName +=  QString::fromUtf8( "_r0" );
    const QString filePath = storagePath() + QDir::separator() + fileName;

    QFile file( filePath );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
      if ( file.write( data ) == data.size() ) {
        part->setData( fileName.toLocal8Bit() );
        result = part->update();
      } else {
        akError() << "Insert: payload file " << filePath << " could not be written to!";
        akError() << "Error: " << file.errorString();
        return false;
      }
      file.close();
    } else {
      akError() << "Insert: payload file " << filePath << " could not be open for writing!";
      akError() << "Error: " << file.errorString();
      return false;
    }
  }
  return result;
}

bool PartHelper::remove( Akonadi::Part *part )
{
  if ( !part ) {
    return false;
  }

  if ( part->external() ) {
    // akDebug() << "remove part file " << part->data();
    const QString fileName = resolveAbsolutePath( part->data() );
    removeFile( fileName );
  }
  return part->remove();
}

bool PartHelper::remove( const QString &column, const QVariant &value )
{
  SelectQueryBuilder<Part> builder;
  builder.addValueCondition( column, Query::Equals, value );
  builder.addValueCondition( Part::externalColumn(), Query::Equals, true );
  builder.addValueCondition( Part::dataColumn(), Query::IsNot, QVariant() );
  if ( !builder.exec() ) {
//      akDebug() << "Error selecting records to be deleted from table"
//          << Part::tableName() << builder.query().lastError().text();
    return false;
  }
  const Part::List parts = builder.result();
  Part::List::ConstIterator it = parts.constBegin();
  Part::List::ConstIterator end = parts.constEnd();
  for ( ; it != end; ++it ) {
    const QString fileName = resolveAbsolutePath( ( *it ).data() );
    // akDebug() << "remove part file " << fileName;
    removeFile( fileName );
  }
  return Part::remove( column, value );
}

void PartHelper::removeFile( const QString &fileName )
{
  if ( !fileName.startsWith( storagePath() ) ) {
    throw PartHelperException( "Attempting to delete a file not in our prefix." );
  }
  QFile::remove( fileName );
}

bool PartHelper::streamToFile( ImapStreamParser* streamParser, QFile &file, QIODevice::OpenMode openMode )
{
  Q_ASSERT( openMode & QIODevice::WriteOnly );

  if ( !file.isOpen() ) {
    if ( !file.open( openMode ) ) {
      throw PartHelperException( "Unable to update item part" );
    }
  } else {
    Q_ASSERT( file.openMode() & QIODevice::WriteOnly );
  }

  QByteArray value;
  while ( !streamParser->atLiteralEnd() ) {
    value = streamParser->readLiteralPart();
    if ( file.write( value ) != value.size() ) {
      throw PartHelperException( "Unable to write payload to file" );
    }
  }
  file.close();

  return true;
}


QByteArray PartHelper::translateData( const QByteArray &data, bool isExternal )
{
  if ( isExternal ) {
    const QString fileName = resolveAbsolutePath( data );

    QFile file( fileName );
    if ( file.open( QIODevice::ReadOnly ) ) {
      const QByteArray payload = file.readAll();
      file.close();
      return payload;
    } else {
      akError() << "Payload file " << fileName << " could not be open for reading!";
      akError() << "Error: " << file.errorString();
      return QByteArray();
    }
  } else {
    // not external
    return data;
  }
}

QByteArray PartHelper::translateData( const Part &part )
{
  return translateData( part.data(), part.external() );
}

bool Akonadi::PartHelper::truncate( Part &part )
{
  if ( part.external() ) {
    const QString fileName = resolveAbsolutePath( part.data() );
    removeFile( fileName );
  }

  part.setData( QByteArray() );
  part.setDatasize( 0 );
  part.setExternal( false );
  return part.update();
}

QString PartHelper::storagePath()
{
  const QString dataDir = AkStandardDirs::saveDir( "data", QLatin1String( "file_db_data" ) ) + QDir::separator();
  Q_ASSERT( dataDir != QDir::separator() );
  return dataDir;
}

bool PartHelper::verify( Part &part )
{
  const QString fileName = resolveAbsolutePath( part.data() );

  if ( !QFile::exists( fileName ) ) {
    akError() << "Payload file" << fileName << "is missing, trying to recover.";
    part.setData( QByteArray() );
    part.setDatasize( 0 );
    part.setExternal( false );
    return part.update();
  }

  return true;
}

QString PartHelper::resolveAbsolutePath( const QByteArray &data )
{
    QString fileName = QString::fromUtf8( data );
    QFileInfo fi( fileName );
    if ( !fi.isAbsolute() ) {
      fileName = storagePath() + fileName;
    }

    return fileName;
}

bool PartHelper::storeStreamedParts( const QByteArray &command,
                                     ImapStreamParser* streamParser,
                                     const PimItem &item, bool checkExists,
                                     QByteArray &partName, qint64 &partSize,
                                     QByteArray &error )
{
 // obtain and configure the part object
  int partVersion = 0;
  partSize = 0;
  ImapParser::splitVersionedKey( command, partName, partVersion );

  const PartType partType = PartTypeHelper::fromFqName( partName );

  Part part;

  if ( checkExists ) {
    SelectQueryBuilder<Part> qb;
    qb.addValueCondition( Part::pimItemIdColumn(), Query::Equals, item.id() );
    qb.addValueCondition( Part::partTypeIdColumn(), Query::Equals, partType.id() );
    if ( !qb.exec() ) {
      error = "Unable to check item part existence";
      return false;
    }

    Part::List result = qb.result();
    if ( !result.isEmpty() ) {
      part = result.first();
    }
  }

  part.setPartType( partType );
  part.setVersion( partVersion );
  part.setPimItemId( item.id() );

  QByteArray value;
  if ( streamParser->hasLiteral() ) {
    const qint64 dataSize = streamParser->remainingLiteralSize();
    if ( partName.startsWith( AKONADI_PARAM_PLD ) ) {
      partSize = dataSize;
    }
    const bool storeInFile = dataSize > DbConfig::configuredDatabase()->sizeThreshold();
    //actual case when streaming storage is used: external payload is enabled, data is big enough in a literal
    if ( storeInFile ) {
      // use first part as value for the initial insert into / update to the database.
      // this will give us a proper filename to stream the rest of the parts contents into
      // NOTE: we have to set the correct size (== dataSize) directly
      value = streamParser->readLiteralPart();
      // akDebug() << Q_FUNC_INFO << "VALUE in STORE: " << value << value.size() << dataSize;

      if ( part.isValid() ) {
        PartHelper::update( &part, value, dataSize );
      } else {
//             akDebug() << "insert from Store::handleLine";
        part.setData( value );
        part.setDatasize( dataSize );
        if ( !PartHelper::insert( &part ) ) {
          error = "Unable to add item part";
          return false;
        }
      }

      //the actual streaming code for the remaining parts:
      // reads from the parser, writes immediately to the file
      QFile partFile( PartHelper::resolveAbsolutePath( part.data() ) );
      try {
        PartHelper::streamToFile( streamParser, partFile, QIODevice::WriteOnly | QIODevice::Append );
      } catch ( const PartHelperException &e ) {
        error = e.what();
        return false;
      }
      return true;
    } else { // not store in file
      //don't write in streaming way as the data goes to the database
      while ( !streamParser->atLiteralEnd() ) {
        value += streamParser->readLiteralPart();
      }
    }
  } else { //not a literal
    value = streamParser->readString();
    if ( partName.startsWith( AKONADI_PARAM_PLD ) ) {
      partSize = value.size();
    }
  }

  // only relevant for non-literals or non-external literals
  const QByteArray origData = PartHelper::translateData( part );
  if ( origData != value ) {
    if ( part.isValid() ) {
      PartHelper::update( &part, value, value.size() );
    } else {
//           akDebug() << "insert from Store::handleLine: " << value.left(100);
      part.setData( value );
      part.setDatasize( value.size() );
      if ( !PartHelper::insert( &part ) ) {
        error = "Unable to add item part";
        return false;
      }
    }
  }

  return true;
}
