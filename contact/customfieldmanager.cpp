/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "customfieldmanager_p.h"

#include <kconfig.h>
#include <kconfiggroup.h>

#include <QVector>

void CustomFieldManager::setGlobalCustomFieldDescriptions( const CustomField::List &customFields )
{
  KConfig config( QLatin1String( "akonadi_contactrc" ) );
  KConfigGroup group( &config, QLatin1String( "GlobalCustomFields" ) );

  group.deleteGroup();
  foreach ( const CustomField &field, customFields ) {
    const QString key = field.key();
    const QString value = CustomField::typeToString( field.type() ) + QLatin1Char( ':' ) + field.title();

    group.writeEntry( key, value );
  }
}

CustomField::List CustomFieldManager::globalCustomFieldDescriptions()
{
  KConfig config( QLatin1String( "akonadi_contactrc" ) );
  const KConfigGroup group( &config, QLatin1String( "GlobalCustomFields" ) );

  CustomField::List customFields;

  const QStringList keys = group.keyList();
  foreach ( const QString &key, keys ) {
    CustomField field;
    field.setKey( key );
    field.setScope( CustomField::GlobalScope );

    const QString value = group.readEntry( key, QString() );
    const int pos = value.indexOf( QLatin1Char( ':' ) );
    if ( pos != -1 ) {
      field.setType( CustomField::stringToType( value.left( pos - 1 ) ) );
      field.setTitle( value.mid( pos + 1 ) );
    }

    customFields << field;
  }

  return customFields;
}
