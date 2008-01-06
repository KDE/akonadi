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

#ifndef AKONADI_COLLECITONPROPERTYPAGE_H
#define AKONADI_COLLECITONPROPERTYPAGE_H

#include <libakonadi/collection.h>

#include <QtGui/QWidget>

namespace Akonadi {

/**
  A single page in a collection property dialog.
  @see Akonadi::CollectionPropertiesDialog, Akonadi::CollectionPropertiesPageFactory
*/
class AKONADI_EXPORT CollectionPropertiesPage : public QWidget
{
  Q_OBJECT
  public:
    /**
      Create a new property page widget.
      @param parent The parent widget.
    */
    explicit CollectionPropertiesPage( QWidget *parent = 0 );

    /**
      Destructor.
    */
    ~CollectionPropertiesPage();

    /**
      Load page content from the given collection.
      @param collection The collection to load.
    */
    virtual void load( const Collection &collection ) = 0;

    /**
      Save page content to the given collection.
      @param collection Reference to the collection to save to.
    */
    virtual void save( Collection &collection ) = 0;

    /**
      Check if this page can actually handle the given collection.
      @return @c true if the collection can be handled, @c false otherwise
      The default implementation returns always @c true. When @c false is returned
      this page is not shown in the properties dialog.
    */
    virtual bool canHandle( const Collection &collection ) const;

    /**
      Returns the page title.
    */
    QString pageTitle() const;

    /**
      Set the page title.
      @param title Translated, preferrably short tab title.
    */
    void setPageTitle( const QString &title );

  private:
    class Private;
    Private* const d;
};

/**
  Factory class for collection property dialog pages.
  You probably want to use Akonadi::CollectionPropertiesPageFactory instead.
  @see Akonadi::CollectionPropertiesPageFactory
*/
class AKONADI_EXPORT AbstractCollectionPropertiesPageFactory
{
  public:
    virtual ~AbstractCollectionPropertiesPageFactory();

    /**
      Returns the actual page widget.
      @param parent The parent widget.
    */
    virtual CollectionPropertiesPage* createWidget( QWidget *parent = 0 ) const = 0;
};


/**
  Factory class for collection property dialog pages.
  @see Akonadi::CollectionPropertiesDialog, Akonadi::CollectionPropertiesPage
*/
template <typename T> class AKONADI_EXPORT CollectionPropertiesPageFactory
  : public AbstractCollectionPropertiesPageFactory
{
  public:
    /**
      Returns the actual page widget.
      @param parent The parent widget.
    */
    inline CollectionPropertiesPage* createWidget( QWidget *parent = 0 ) const
    {
      return new T( parent );
    }
};

}

#endif
