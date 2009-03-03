/*
    Copyright (c) Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on  kdepim/akonadi/resources/knut

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

#ifndef AKONADIXML_H
#define AKONADIXML_H

#include "akonadi-xml_export.h"

#include <QDomDocument>

#include <akonadi/collection.h>
#include <akonadi/item.h>


class AKONADI_XML_EXPORT AkonadiXML {

  public:
    void deserializeAttributes( const QDomElement &node, Akonadi::Entity &entity );
    Akonadi::Collection buildCollection( const QDomElement &node, const QString &parentRid );
    Akonadi::Collection::List buildCollectionTree( const QDomElement &parent );
    void addItemPayload( Akonadi::Item &item, const QDomElement &elem );
    Akonadi::Item buildItem( const QDomElement &elem );
    QDomElement serializeItem( Akonadi::Item &item );
    void serializeAttributes( const Akonadi::Entity &entity, QDomElement &entityElem );
    QDomElement serializeCollection( Akonadi::Collection &collection );

  private:
    QDomDocument mDocument;
};

#endif
