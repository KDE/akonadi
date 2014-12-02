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

#ifndef AKONADI_COLLECTIONPROPERTIESPAGE_H
#define AKONADI_COLLECTIONPROPERTIESPAGE_H

#include "akonadiwidgets_export.h"

#include <QWidget>

namespace Akonadi {

class Collection;

/**
 * @short A single page in a collection properties dialog.
 *
 * The collection properties dialog can be extended by custom
 * collection properties pages, which provide gui elements for
 * viewing and changing collection attributes.
 *
 * The following example shows how to create a simple collection
 * properties page for the secrecy attribute from the Akonadi::Attribute
 * example.
 *
 * @code
 *
 * class SecrecyPage : public CollectionPropertiesPage
 * {
 *    public:
 *      SecrecyPage( QWidget *parent = Q_NULLPTR )
 *        : CollectionPropertiesPage( parent )
 *      {
 *        QVBoxLayout *layout = new QVBoxLayout( this );
 *
 *        mSecrecy = new QComboBox( this );
 *        mSecrecy->addItem( "Public" );
 *        mSecrecy->addItem( "Private" );
 *        mSecrecy->addItem( "Confidential" );
 *
 *        layout->addWidget( new QLabel( "Secrecy:" ) );
 *        layout->addWidget( mSecrecy );
 *
 *        setPageTitle( i18n( "Secrecy" ) );
 *      }
 *
 *      void load( const Collection &collection )
 *      {
 *        SecrecyAttribute *attr = collection.attribute( "secrecy" );
 *
 *        switch ( attr->secrecy() ) {
 *          case SecrecyAttribute::Public: mSecrecy->setCurrentIndex( 0 ); break;
 *          case SecrecyAttribute::Private: mSecrecy->setCurrentIndex( 1 ); break;
 *          case SecrecyAttribute::Confidential: mSecrecy->setCurrentIndex( 2 ); break;
 *        }
 *      }
 *
 *      void save( Collection &collection )
 *      {
 *        SecrecyAttribute *attr = collection.attribute( "secrecy" );
 *
 *        switch ( mSecrecy->currentIndex() ) {
 *          case 0: attr->setSecrecy( SecrecyAttribute::Public ); break;
 *          case 1: attr->setSecrecy( SecrecyAttribute::Private ); break;
 *          case 2: attr->setSecrecy( SecrecyAttribute::Confidential ); break;
 *        }
 *      }
 *
 *      bool canHandle( const Collection &collection ) const
 *      {
 *        return collection.hasAttribute( "secrecy" );
 *      }
 * };
 *
 * AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( SecrecyPageFactory, SecrecyPage )
 *
 * @endcode
 *
 * @see Akonadi::CollectionPropertiesDialog, Akonadi::CollectionPropertiesPageFactory
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionPropertiesPage : public QWidget
{
    Q_OBJECT
public:
    /**
     * Creates a new collection properties page.
     *
     * @param parent The parent widget.
     */
    explicit CollectionPropertiesPage(QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the collection properties page.
     */
    ~CollectionPropertiesPage();

    /**
     * Loads the page content from the given collection.
     *
     * @param collection The collection to load.
     */
    virtual void load(const Collection &collection) = 0;

    /**
     * Saves page content to the given collection.
     *
     * @param collection Reference to the collection to save to.
     */
    virtual void save(Collection &collection) = 0;

    /**
     * Checks if this page can actually handle the given collection.
     *
     * Returns @c true if the collection can be handled, @c false otherwise
     * The default implementation returns always @c true. When @c false is returned
     * this page is not shown in the properties dialog.
     * @param collection The collection to check.
     */
    virtual bool canHandle(const Collection &collection) const;

    /**
     * Sets the page title.
     *
     * @param title Translated, preferbly short tab title.
     */
    void setPageTitle(const QString &title);

    /**
     * Returns the page title.
     */
    QString pageTitle() const;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

/**
 * @short A factory class for collection properties dialog pages.
 *
 * The factory encapsulates the creation of the collection properties
 * dialog page.
 * You can use the AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY macro
 * to create a factory class automatically.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionPropertiesPageFactory
{
public:
    /**
     * Destroys the collection properties page factory.
     */
    virtual ~CollectionPropertiesPageFactory();

    /**
     * Returns the actual page widget.
     *
     * @param parent The parent widget.
     */
    virtual CollectionPropertiesPage *createWidget(QWidget *parent = Q_NULLPTR) const = 0;
};

/**
 * @def AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY
 *
 * The AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY macro can be used to
 * create a factory for a custom collection properties page.
 *
 * @code
 *
 * class MyPage : public Akonadi::CollectionPropertiesPage
 * {
 *   ...
 * }
 *
 * AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( MyPageFactory, MyPage )
 *
 * @endcode
 *
 * The macro takes two arguments, where the first one is the name of the
 * factory class that shall be created and the second arguments is the name
 * of the custom collection properties page class.
 *
 * @ingroup AkonadiMacros
 */
#define AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(factoryName, className) \
class factoryName : public Akonadi::CollectionPropertiesPageFactory \
{ \
  public: \
    inline Akonadi::CollectionPropertiesPage *createWidget( QWidget *parent = Q_NULLPTR ) const \
    { \
      return new className( parent ); \
    } \
};

}

#endif
