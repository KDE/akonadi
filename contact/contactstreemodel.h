/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>
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

#ifndef AKONADI_CONTACTSTREEMODEL_H
#define AKONADI_CONTACTSTREEMODEL_H

#include "akonadi-contact_export.h"

#include <akonadi/entitytreemodel.h>

namespace Akonadi {

/**
 * @short A model for contacts and contact groups as available in Akonadi.
 *
 * This class provides a model for displaying the contacts and
 * contact groups which are available from Akonadi.
 *
 * Example:
 *
 * @code
 *
 * // use a separated session for this model
 * Akonadi::Session *session = new Akonadi::Session( "MySession" );
 *
 * Akonadi::ItemFetchScope scope;
 * // fetch all content of the contacts, including images
 * scope.fetchFullPayload( true );
 * // fetch the EntityDisplayAttribute, which contains custom names and icons
 * scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();
 *
 * Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder;
 * changeRecorder->setSession( session );
 * // include fetching the collection tree
 * changeRecorder->fetchCollection( true );
 * // set the fetch scope that shall be used
 * changeRecorder->setItemFetchScope( scope );
 * // monitor all collections below the root collection for changes
 * changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );
 * // list only contacts and contact groups
 * changeRecorder->setMimeTypeMonitored( KABC::Addressee::mimeType(), true );
 * changeRecorder->setMimeTypeMonitored( KABC::ContactGroup::mimeType(), true );
 *
 * Akonadi::ContactsTreeModel *model = new Akonadi::ContactsTreeModel( changeRecorder );
 *
 * Akonadi::ContactsTreeModel::Columns columns;
 * columns << Akonadi::ContactsTreeModel::FullName;
 * columns << Akonadi::ContactsTreeModel::AllEmails;
 * model->setColumns( columns );
 *
 * Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView;
 * view->setModel( model );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT ContactsTreeModel : public EntityTreeModel
{
  Q_OBJECT

  public:
    /**
     * Describes the columns that can be shown by the model.
     */
    enum Column {
      /**
       * Shows the formatted name or, if empty, the assembled name.
       */
      FullName,

      /**
       * Shows the family name.
       */
      FamilyName,

      /**
       * Shows the given name.
       */
      GivenName,

      /**
       * Shows the birthday.
       */
      Birthday,

      /**
       * Shows the formatted home address.
       */
      HomeAddress,

      /**
       * Shows the formatted business address.
       */
      BusinessAddress,

      /**
       * Shows the phone numbers.
       */
      PhoneNumbers,

      /**
       * Shows the preferred email address.
       */
      PreferredEmail,

      /**
       * Shows all email address.
       */
      AllEmails,

      /**
       * Shows organization name.
       */
      Organization,

      /**
       * Shows the role of a contact.
       */
      Role,

      /**
       * Shows homepage url.
       */
      Homepage,

      /**
       * Shows the note.
       */
      Note
    };

    /**
     * Describes a list of columns of the contacts tree model.
     */
    typedef QList<Column> Columns;

    /**
     * Describes the role for contacts and contact groups.
     */
    enum Roles {
      DateRole = EntityTreeModel::UserRole + 1,   ///< The QDate object for the current index.
      UserRole = DateRole + 42
    };

    /**
     * Creates a new contacts tree model.
     *
     * @param monitor The ChangeRecorder whose entities should be represented in the model.
     * @param parent The parent object.
     */
    explicit ContactsTreeModel( ChangeRecorder *monitor, QObject *parent = 0 );

    /**
     * Destroys the contacts tree model.
     */
    virtual ~ContactsTreeModel();

    /**
     * Sets the @p columns that the model should show.
     */
    void setColumns( const Columns &columns );

    /**
     * Returns the columns that the model currently shows.
     */
    Columns columns() const;

    //@cond PRIVATE
    virtual QVariant entityData( const Item &item, int column, int role = Qt::DisplayRole ) const;
    virtual QVariant entityData( const Collection &collection, int column, int role = Qt::DisplayRole ) const;
    virtual QVariant entityHeaderData( int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup ) const;
    virtual int entityColumnCount( HeaderGroup headerGroup ) const;
    //@endcond

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
