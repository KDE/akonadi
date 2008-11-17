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
#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
#include <kabc/vcardformat.h>
#include <kabc/addressbook.h>
#include <kabc/format.h>
#include <kabc/resource.h>

VCardItem::VCardItem(QFile *file, const QString &mimetype)
:Item(mimetype) {
  KABC::VCardConverter converter;
  
  QByteArray data = file->readAll();
  vcardlist = converter.parseVCards( data );
  
  foreach(const KABC::Addressee &tmp, vcardlist){
    Akonadi::Item tmpitem;
    tmpitem.setMimeType(mimetype);
    tmpitem.setPayload<KABC::Addressee>( tmp );
    item.append( tmpitem  );
  }
  
}


