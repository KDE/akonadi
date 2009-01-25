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


#include "vcarditem.h"

#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>

#include <QtCore/QFile>

VCardItem::VCardItem( const QString &fileName, const QString &mimetype )
  : Item( mimetype )
{
  KABC::VCardConverter converter;

  QFile file( fileName );
  file.open( QIODevice::ReadOnly );

  const QByteArray data = file.readAll();
  const KABC::Addressee::List addressees = converter.parseVCards( data );

  foreach ( const KABC::Addressee &addressee, addressees ) {
    Akonadi::Item item;
    item.setMimeType( mimetype );
    item.setPayload<KABC::Addressee>( addressee );
    mItems.append( item  );
  }
}
