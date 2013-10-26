/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialmailcollectionsrequestjob.h"

#include "specialmailcollections.h"

#include <kglobal.h>
#include <klocalizedstring.h>
#include <kstandarddirs.h>

using namespace Akonadi;

#ifndef KDE_USE_FINAL
static inline QByteArray enumToType( SpecialMailCollections::Type type )
{
  switch ( type ) {
    case SpecialMailCollections::Root: return "local-mail"; break;
    case SpecialMailCollections::Inbox: return "inbox"; break;
    case SpecialMailCollections::Outbox: return "outbox"; break;
    case SpecialMailCollections::SentMail: return "sent-mail"; break;
    case SpecialMailCollections::Trash: return "trash"; break;
    case SpecialMailCollections::Drafts: return "drafts"; break;
    case SpecialMailCollections::Templates: return "templates"; break;
    case SpecialMailCollections::LastType: // fallthrough
    default: return QByteArray(); break;
  }
}
#endif

SpecialMailCollectionsRequestJob::SpecialMailCollectionsRequestJob( QObject *parent )
  : SpecialCollectionsRequestJob( SpecialMailCollections::self(), parent ),
    d( 0 )
{
  static QMap<QByteArray, QString> displayNameMap;
  displayNameMap.insert( "local-mail", i18nc( "local mail folder", "Local Folders" ) );
  displayNameMap.insert( "inbox", /*i18nc( "local mail folder",*/QLatin1String("inbox" ) );
  displayNameMap.insert( "outbox", /*i18nc( "local mail folder",*/ QLatin1String("outbox" ) );
  displayNameMap.insert( "sent-mail", /*i18nc( "local mail folder",*/QLatin1String( "sent-mail" ) );
  displayNameMap.insert( "trash", /*i18nc( "local mail folder", */QLatin1String("trash" ) );
  displayNameMap.insert( "drafts", /*i18nc( "local mail folder", */QLatin1String("drafts" ) );
  displayNameMap.insert( "templates", /*i18nc( "local mail folder", */QLatin1String("templates" ) );

  static QMap<QByteArray, QString> iconNameMap;
  iconNameMap.insert( "local-mail", QLatin1String( "folder" ) );
  iconNameMap.insert( "inbox", QLatin1String( "mail-folder-inbox" ) );
  iconNameMap.insert( "outbox", QLatin1String( "mail-folder-outbox" ) );
  iconNameMap.insert( "sent-mail", QLatin1String( "mail-folder-sent" ) );
  iconNameMap.insert( "trash", QLatin1String( "user-trash" ) );
  iconNameMap.insert( "drafts", QLatin1String( "document-properties" ) );
  iconNameMap.insert( "templates", QLatin1String( "document-new" ) );

  QVariantMap options;
  options.insert( QLatin1String( "Name" ), displayNameMap.value( "local-mail" ) );
  options.insert( QLatin1String( "TopLevelIsContainer" ), true );
  options.insert( QLatin1String( "Path" ), QString(KGlobal::dirs()->localxdgdatadir() + QLatin1String( "local-mail" )) );

  setDefaultResourceType( QLatin1String( "akonadi_maildir_resource" ) );
  setDefaultResourceOptions( options );

  setTypes( displayNameMap.keys() );
  setNameForTypeMap( displayNameMap );
  setIconForTypeMap( iconNameMap );
}

SpecialMailCollectionsRequestJob::~SpecialMailCollectionsRequestJob()
{
}

void SpecialMailCollectionsRequestJob::requestDefaultCollection( SpecialMailCollections::Type type )
{
  return SpecialCollectionsRequestJob::requestDefaultCollection( enumToType( type ) );
}

void SpecialMailCollectionsRequestJob::requestCollection( SpecialMailCollections::Type type, const AgentInstance &instance )
{
  return SpecialCollectionsRequestJob::requestCollection( enumToType( type ), instance );
}

