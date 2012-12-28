/*
    This file is part of Akonadi Contact.

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

#ifndef AKONADI_CONTACTSFILTERPROXYMODEL_H
#define AKONADI_CONTACTSFILTERPROXYMODEL_H

#include "akonadi-contact_export.h"

#include <QSortFilterProxyModel>

namespace Akonadi {

/**
 * @short A proxy model for \a ContactsTreeModel models.
 *
 * This class provides a filter proxy model for a ContactsTreeModel.
 * The list of shown contacts or contact groups can be limited by
 * settings a filter pattern. Only contacts or contact groups that contain
 * this pattern as part of their data will be listed.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::ContactsTreeModel *model = new Akonadi::ContactsTreeModel( ... );
 *
 * Akonadi::ContactsFilterProxyModel *filter = new Akonadi::ContactsFilterProxyModel;
 * filter->setSourceModel( model );
 *
 * Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView;
 * view->setModel( filter );
 *
 * QLineEdit *filterEdit = new QLineEdit;
 * connect( filterEdit, SIGNAL( textChanged( const QString& ) ),
 *          filter, SLOT( setFilterString( const QString& ) ) );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT ContactsFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:

    enum FilterFlag {
      HasEmail = 0x01 /// Filters out contacts without any email address set.
    };
    Q_DECLARE_FLAGS( FilterFlags, FilterFlag )

    /**
     * Creates a new contacts filter proxy model.
     *
     * @param parent The parent object.
     */
    explicit ContactsFilterProxyModel( QObject *parent = 0 );

    /**
     * Destroys the contacts filter proxy model.
     */
    virtual ~ContactsFilterProxyModel();

     /**
     * Sets the filter @p flags. By default
     * ContactsFilterProxyModel::FilterString is set.
     * @param flags the filter flags to set
     * @since 4.8
     */
     void setFilterFlags( ContactsFilterProxyModel::FilterFlags flags );

     virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    /**
     * Sets whether we want virtual collections to be filtered or not.
     * By default, virtual collections are accepted.
     *
     * @param exclude If true, virtual collections aren't accepted.
     *
     * @since 4.8
     */
     void setExcludeVirtualCollections( bool exclude );

  public Q_SLOTS:
    /**
     * Sets the @p filter that is used to filter for matching contacts
     * and contact groups.
     */
    void setFilterString( const QString &filter );

  protected:
    //@cond PRIVATE
    virtual bool filterAcceptsRow( int row, const QModelIndex &parent ) const;
    virtual bool lessThan( const QModelIndex &left, const QModelIndex &right ) const;
    //@endcond

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

Q_DECLARE_OPERATORS_FOR_FLAGS ( ContactsFilterProxyModel::FilterFlags )

}

#endif
