/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKOANDI_COLLECTIONPROPERTYGENERALPAGE_H
#define AKOANDI_COLLECTIONPROPERTYGENERALPAGE_H

#include "collectionpropertiespage.h"
#include "ui_collectiongeneralpropertiespage.h"

namespace Akonadi {

/**
  General page of the collection property dialog.
  @internal
*/
class CollectionGeneralPropertiesPage : public CollectionPropertiesPage
{
  Q_OBJECT
  public:
    explicit CollectionGeneralPropertiesPage( QWidget *parent = 0 );

    void load( const Collection &collection );
    void save( Collection &collection );

  private:
    Ui::CollectionGeneralPropertiesPage ui;
};

}

#endif
