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
#include "akonadi/xml/xmlwriter.h"
#include "akonadi/xml/xmlreader.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <QtCore/QFile>
#include <QFileSystemWatcher>
#include <QtCore/QDir>
#include <QtCore/QUuid>

using namespace Akonadi;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id ),
  mWatcher( new QFileSystemWatcher( this ) )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(load()) );
  connect( mWatcher, SIGNAL(fileChanged(QString)), SLOT(load()) );
  load();
}

KnutResource::~KnutResource()
{
}

void KnutResource::load()
{
  if ( !mWatcher->files().isEmpty() )
    mWatcher->removePaths( mWatcher->files() );

  // file loading
  QString fileName = Settings::self()->dataFile();
  if ( fileName.isEmpty() ) {
    emit status( Broken, i18n( "No data file selected." ) );
    return;
  }

  if ( !QFile::exists( fileName ) )
    fileName = KGlobal::dirs()->findResource( "data", QLatin1String("akonadi_knut_resource/knut-template.xml") );

  if( !mDocument.loadFile(fileName) ) {
    emit status( Broken, mDocument.lastError() );
    return;
  }

  if ( Settings::self()->fileWatchingEnabled() )
    mWatcher->addPath( fileName );

  emit status( Idle, i18n( "File '%1' loaded successfully.", fileName ) );
  synchronize();
}

void KnutResource::save()
{
  if ( Settings::self()->readOnly() )
    return;
  const QString fileName = Settings::self()->dataFile();
  if  ( !mDocument.writeToFile( fileName ) ) {
    emit error( mDocument.lastError() );
    return;
  }
}


void KnutResource::configure( WId windowId )
{
  QString newFile;

  QString oldFile = Settings::self()->dataFile();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  newFile = KFileDialog::getSaveFileNameWId( url, "*.xml |" + i18nc( "Filedialog filter for Akonadi data file", "Akonadi Knut Data File" ), windowId, i18n( "Select Data File" ) );

  if ( newFile.isEmpty() || oldFile == newFile )
    return;

  Settings::self()->setDataFile( newFile );
  Settings::self()->writeConfig();
  load();
}

void KnutResource::retrieveCollections()
{
  const Collection::List collections = mDocument.collections();
  collectionsRetrieved( collections );
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
    cancelTask( "No item found for remoteid " + item.remoteId() );
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
    emit error( "Parent collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Collection c( collection );
  QDomElement newCol;
  XmlWriter::writeCollection( c, newCol );
  parentElem.insertBefore( newCol, QDomNode() );
  save();
  changeCommitted( c );
}

void KnutResource::collectionChanged( const Akonadi::Collection &collection )
{
  QDomElement oldElem = mDocument.collectionElementByRemoteId( collection.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( "Modified collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Collection c( collection );
  QDomElement newElem;
  XmlWriter::writeCollection( c, newElem );
  // move all items/collections over to the new node
  const QDomNodeList children = oldElem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    kDebug() << "reparenting " << child.tagName() << child.attribute( "rid" );
    if ( child.isNull() )
      continue;
    if ( child.tagName() == "item" || child.tagName() == "collection" ) {
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
    emit error( "Deleted collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  colElem.parentNode().removeChild( colElem );
  save();
  changeProcessed();
}


void KnutResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  QDomElement parentElem = mDocument.itemElementByRemoteId( collection.remoteId() );
  if ( parentElem.isNull() ) {
    emit error( "Parent collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Item i( item );
  const QDomElement newItem = XmlWriter::itemToElement(i, mDocument.document()); //serializeItem( i );
  parentElem.appendChild( newItem );
  save();
  changeCommitted( i );
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QDomElement oldElem = mDocument.itemElementByRemoteId( item.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( "Modified item not found in DOM tree." );
    changeProcessed();
    return;
  }

  Item i( item );
  const QDomElement newElem = XmlWriter::itemToElement(i, mDocument.document());
  oldElem.parentNode().replaceChild( newElem, oldElem );
  save();
  changeCommitted( i );
}

void KnutResource::itemRemoved( const Akonadi::Item &item )
{
  const QDomElement itemElem = mDocument.itemElementByRemoteId( item.remoteId() );
  if ( itemElem.isNull() ) {
    emit error( "Deleted item not found in DOM tree." );
    changeProcessed();
    return;
  }

  itemElem.parentNode().removeChild( itemElem );
  save();
  changeProcessed();
}

AKONADI_RESOURCE_MAIN( KnutResource )

#include "knutresource.moc"
