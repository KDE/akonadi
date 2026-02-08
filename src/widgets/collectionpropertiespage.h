/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

#include <QWidget>

#include <memory>

namespace Akonadi
{
class Collection;
class CollectionPropertiesPagePrivate;

/*!
 * \brief A single page in a collection properties dialog.
 *
 * The collection properties dialog can be extended by custom
 * collection properties pages, which provide gui elements for
 * viewing and changing collection attributes.
 *
 * The following example shows how to create a simple collection
 * properties page for the secrecy attribute from the Akonadi::Attribute
 * example.
 *
 * \code
 *
 * class SecrecyPage : public CollectionPropertiesPage
 * {
 *    public:
 *      SecrecyPage( QWidget *parent = nullptr )
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
 * \endcode
 *
 * \sa Akonadi::CollectionPropertiesDialog, Akonadi::CollectionPropertiesPageFactory
 *
 * \class Akonadi::CollectionPropertiesPage
 * \inheaderfile Akonadi/CollectionPropertiesPage
 * \inmodule AkonadiWidgets
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionPropertiesPage : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new collection properties page.
     *
     * \a parent The parent widget.
     */
    explicit CollectionPropertiesPage(QWidget *parent = nullptr);

    /*!
     * Destroys the collection properties page.
     */
    ~CollectionPropertiesPage() override;

    /*!
     * Loads the page content from the given collection.
     *
     * \a collection The collection to load.
     */
    virtual void load(const Collection &collection) = 0;

    /*!
     * Saves page content to the given collection.
     *
     * \a collection Reference to the collection to save to.
     */
    virtual void save(Collection &collection) = 0;

    /*!
     * Checks if this page can actually handle the given collection.
     *
     * Returns \\ true if the collection can be handled, \\ false otherwise
     * The default implementation returns always \\ true. When \\ false is returned
     * this page is not shown in the properties dialog.
     * \a collection The collection to check.
     */
    virtual bool canHandle(const Collection &collection) const;

    /*!
     * Sets the page title.
     *
     * \a title Translated, preferably short tab title.
     */
    void setPageTitle(const QString &title);

    /*!
     * Returns the page title.
     */
    [[nodiscard]] QString pageTitle() const;

private:
    std::unique_ptr<CollectionPropertiesPagePrivate> const d;
};

/*!
 * \brief A factory class for collection properties dialog pages.
 *
 * The factory encapsulates the creation of the collection properties
 * dialog page.
 * You can use the AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY macro
 * to create a factory class automatically.
 *
 * \class Akonadi::CollectionPropertiesPageFactory
 * \inheaderfile Akonadi/CollectionPropertiesPage
 * \inmodule AkonadiWidgets
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionPropertiesPageFactory
{
public:
    /*!
     * Destroys the collection properties page factory.
     */
    virtual ~CollectionPropertiesPageFactory();

    /*!
     * Returns the actual page widget.
     *
     * \a parent The parent widget.
     */
    virtual CollectionPropertiesPage *createWidget(QWidget *parent = nullptr) const = 0;

protected:
    explicit CollectionPropertiesPageFactory() = default;

private:
    Q_DISABLE_COPY_MOVE(CollectionPropertiesPageFactory)
};

/*!
 * @def AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY
 *
 * The AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY macro can be used to
 * create a factory for a custom collection properties page.
 *
 * \code
 *
 * class MyPage : public Akonadi::CollectionPropertiesPage
 * {
 *   ...
 * }
 *
 * AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY( MyPageFactory, MyPage )
 *
 * \endcode
 *
 * The macro takes two arguments, where the first one is the name of the
 * factory class that shall be created and the second arguments is the name
 * of the custom collection properties page class.
 *
 * @ingroup AkonadiMacros
 */
#define AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(factoryName, className)                                                                                     \
    class factoryName : public Akonadi::CollectionPropertiesPageFactory                                                                                        \
    {                                                                                                                                                          \
    public:                                                                                                                                                    \
        inline Akonadi::CollectionPropertiesPage *createWidget(QWidget *parent = nullptr) const override                                                       \
        {                                                                                                                                                      \
            return new className(parent);                                                                                                                      \
        }                                                                                                                                                      \
    };

}
