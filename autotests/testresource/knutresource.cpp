/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "knutresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "xml/xmlwriter.h"
#include "xml/xmlreader.h"
#include <searchquery.h>

#include <akonadi/agentfactory.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/dbusconnectionpool.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/tagcreatejob.h>

#include <kfiledialog.h>
#include <klocale.h>


#include <QtCore/QFile>
#include <QFileSystemWatcher>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QStandardPaths>

using namespace Akonadi;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id ),
  mWatcher( new QFileSystemWatcher( this ) ),
  mSettings( new KnutSettings( componentData().config() ) )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  new SettingsAdaptor( mSettings );
  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/Settings" ),
                               mSettings, QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(load()) );
  connect( mWatcher, SIGNAL(fileChanged(QString)), SLOT(load()) );
  load();
}

KnutResource::~KnutResource()
{
  delete mSettings;
}

void KnutResource::load()
{
  if ( !mWatcher->files().isEmpty() )
    mWatcher->removePaths( mWatcher->files() );

  // file loading
  QString fileName = mSettings->dataFile();
  if ( fileName.isEmpty() ) {
    emit status( Broken, i18n( "No data file selected." ) );
    return;
  }

  if ( !QFile::exists( fileName ) )
    fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String( "akonadi_knut_resource/knut-template.xml" ) );

  if ( !mDocument.loadFile( fileName ) ) {
    emit status( Broken, mDocument.lastError() );
    return;
  }

  if ( mSettings->fileWatchingEnabled() )
    mWatcher->addPath( fileName );

  emit status( Idle, i18n( "File '%1' loaded successfully.", fileName ) );
  synchronize();
}

void KnutResource::save()
{
  if ( mSettings->readOnly() )
    return;
  const QString fileName = mSettings->dataFile();
  if  ( !mDocument.writeToFile( fileName ) ) {
    emit error( mDocument.lastError() );
    return;
  }
}


void KnutResource::configure( WId windowId )
{
  const QString oldFile = mSettings->dataFile();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  const QString newFile =
      KFileDialog::getSaveFileNameWId( url,
                                       QStringLiteral("*.xml |") + i18nc( "Filedialog filter for Akonadi data file", "Akonadi Knut Data File" ),
                                       windowId, i18n( "Select Data File" ) );

  if ( newFile.isEmpty() || oldFile == newFile )
    return;

  mSettings->setDataFile( newFile );
  mSettings->writeConfig();
  load();

  emit configurationDialogAccepted();
}

void KnutResource::retrieveCollections()
{
  const Collection::List collections = mDocument.collections();
  collectionsRetrieved( collections );
  const Tag::List tags = mDocument.tags();
  Q_FOREACH ( const Tag &tag, tags ) {
      TagCreateJob *createjob = new TagCreateJob(tag);
      createjob->setMergeIfExisting(true);
  }
}

void KnutResource::retrieveItems( const Akonadi::Collection &collection )
{
  Item::List items = mDocument.items( collection, false );
  if ( !mDocument.lastError().isEmpty() ) {
    cancelTask( mDocument.lastError() );
    return;
  }

  itemsRetrieved( items );
}


bool KnutResource::retrieveItem( const Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  const QDomElement itemElem = mDocument.itemElementByRemoteId( item.remoteId() );
  if ( itemElem.isNull() ) {
    cancelTask( i18n( "No item found for remoteid %1", item.remoteId() ) );
    return false;
  }

  Item i = XmlReader::elementToItem( itemElem, true );
  i.setId( item.id() );
  itemRetrieved( i );
  return true;
}


void KnutResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  QDomElement parentElem = mDocument.collectionElementByRemoteId( parent.remoteId() );
  if ( parentElem.isNull() ) {
    emit error( i18n( "Parent collection not found in DOM tree." ) );
    changeProcessed();
    return;
  }

  Collection c( collection );
  c.setRemoteId( QUuid::createUuid().toString() );
  if ( XmlWriter::writeCollection( c, parentElem ).isNull() ) {
    emit error( i18n( "Unable to write collection." ) );
    changeProcessed();
  } else {
    save();
    changeCommitted( c );
  }
}

void KnutResource::collectionChanged( const Akonadi::Collection &collection )
{
  QDomElement oldElem = mDocument.collectionElementByRemoteId( collection.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( i18n( "Modified collection not found in DOM tree." ) );
    changeProcessed();
    return;
  }

  Collection c( collection );
  QDomElement newElem;
  newElem = XmlWriter::collectionToElement( c, mDocument.document() );
  // move all items/collections over to the new node
  const QDomNodeList children = oldElem.childNodes();
  const int numberOfChildren = children.count();
  for ( int i = 0; i < numberOfChildren; ++i ) {
    const QDomElement child = children.at( i ).toElement();
    qDebug() << "reparenting " << child.tagName() << child.attribute( QStringLiteral("rid") );
    if ( child.isNull() )
      continue;
    if ( child.tagName() == QLatin1String("item") || child.tagName() == QLatin1String("collection") ) {
      newElem.appendChild( child ); // reparents
      --i; // children, despite being const is modified by the reparenting
    }
  }
  oldElem.parentNode().replaceChild( newElem, oldElem );
  save();
  changeCommitted( c );
}

void KnutResource::collectionRemoved( const Akonadi::Collection &collection )
{
  const QDomElement colElem = mDocument.collectionElementByRemoteId( collection.remoteId() );
  if ( colElem.isNull() ) {
    emit error( i18n( "Deleted collection not found in DOM tree." ) );
    changeProcessed();
    return;
  }

  colElem.parentNode().removeChild( colElem );
  save();
  changeProcessed();
}


void KnutResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  QDomElement parentElem = mDocument.collectionElementByRemoteId( collection.remoteId() );
  if ( parentElem.isNull() ) {
    emit error( i18n( "Parent collection '%1' not found in DOM tree." , collection.remoteId() ) );
    changeProcessed();
    return;
  }

  Item i( item );
  i.setRemoteId( QUuid::createUuid().toString() );
  if ( XmlWriter::writeItem( i, parentElem ).isNull() ) {
    emit error( i18n( "Unable to write item." ) );
    changeProcessed();
  } else {
    save();
    changeCommitted( i );
  }
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QDomElement oldElem = mDocument.itemElementByRemoteId( item.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( i18n( "Modified item not found in DOM tree." ) );
    changeProcessed();
    return;
  }

  Item i( item );
  const QDomElement newElem = XmlWriter::itemToElement( i, mDocument.document() );
  oldElem.parentNode().replaceChild( newElem, oldElem );
  save();
  changeCommitted( i );
}

void KnutResource::itemRemoved( const Akonadi::Item &item )
{
  const QDomElement itemElem = mDocument.itemElementByRemoteId( item.remoteId() );
  if ( itemElem.isNull() ) {
    emit error( i18n( "Deleted item not found in DOM tree." ) );
    changeProcessed();
    return;
  }

  itemElem.parentNode().removeChild( itemElem );
  save();
  changeProcessed();
}

QSet<qint64> KnutResource::parseQuery(const QString &queryString)
{
  QSet<qint64> resultSet;
  Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON(queryString.toLatin1());
  foreach (const Akonadi::SearchTerm &term, query.term().subTerms()) {
      if (term.key() == QLatin1String("resource")) {
          resultSet << term.value().toInt();
      }
  }
  return resultSet;
}

void KnutResource::search(const QString& query, const Collection& collection)
{
  qDebug() << query;
  searchFinished(parseQuery(query).toList().toVector(), Akonadi::AgentSearchInterface::Uid);
}

void KnutResource::addSearch(const QString& query, const QString& queryLanguage, const Collection& resultCollection)
{
  qDebug();
}

void KnutResource::removeSearch(const Collection& resultCollection)
{
  qDebug();
}


#include "knutresource.moc"
