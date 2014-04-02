/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "recentcontactscollectionrequestjob.h"

#include "recentcontactscollections_p.h"

#include <kglobal.h>
#include <klocalizedstring.h>
#include <QStandardPaths>


using namespace Akonadi;

#ifndef KDE_USE_FINAL
static const QByteArray sRecentContactsType = "recent-contacts";
#endif

RecentContactsCollectionRequestJob::RecentContactsCollectionRequestJob( QObject *parent )
  : SpecialCollectionsRequestJob( RecentContactsCollections::self(), parent ),
    d( 0 )
{
  static QMap<QByteArray, QString> displayNameMap;
  displayNameMap.insert( "recent-contacts", i18nc( "recent contacts folder", "Recent Contacts" ) );

  static QMap<QByteArray, QString> iconNameMap;
  iconNameMap.insert( "recent-contacts", QLatin1String( "folder" ) );

  QVariantMap options;
  options.insert( QLatin1String( "Name" ), displayNameMap.value( "recent-contacts" ) );
  options.insert( QLatin1String( "Path" ), QString( QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + QLatin1String( "recent-contacts" ) ) );

  setDefaultResourceType( QLatin1String( "akonadi_contacts_resource" ) );
  setDefaultResourceOptions( options );

  setTypes( displayNameMap.keys() );
  setNameForTypeMap( displayNameMap );
  setIconForTypeMap( iconNameMap );
}

RecentContactsCollectionRequestJob::~RecentContactsCollectionRequestJob()
{
}

void RecentContactsCollectionRequestJob::requestDefaultCollection()
{
  SpecialCollectionsRequestJob::requestDefaultCollection( sRecentContactsType );
}

void RecentContactsCollectionRequestJob::requestCollection( const AgentInstance &instance )
{
  SpecialCollectionsRequestJob::requestCollection( sRecentContactsType, instance );
}

