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

#ifndef AKONADI_COLLECTIONPROPERTIESDIALOG_H
#define AKONADI_COLLECTIONPROPERTIESDIALOG_H

#include <libakonadi/collection.h>
#include <libakonadi/collectionpropertiespage.h>


#include <kdialog.h>

namespace Akonadi {

/**
  A generic and extensible collection properties dialog.
  @see Akonadi::CollectionPropertiesPage
*/
class AKONADI_EXPORT CollectionPropertiesDialog : public KDialog
{
  Q_OBJECT
  public:
    /**
      Create a new collection properties dialog.
      @param collection The collection which properties should be shown.
      @param parent The parent widget.
    */
    explicit CollectionPropertiesDialog( const Collection &collection, QWidget *parent = 0 );

    /**
      Destructor.
      Never call manually, the dialog is deleted automatically once all changes
      are written back to the Akonadi server.
    */
    ~CollectionPropertiesDialog();

    /**
      Register additional pages for the collection properties dialog.
    */
    template <typename T> static void registerPage()
    {
      registerPage( new CollectionPropertiesPageFactory<T> );
    }

  private:
    static void registerPage( AbstractCollectionPropertiesPageFactory *factory );

  private:
    class Private;
    Private* const d;
    Q_PRIVATE_SLOT( d, void save() )
    Q_PRIVATE_SLOT( d, void saveResult(KJob*) )
};

}

#endif
