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

#ifndef AKONADI_CONTACTSFILTERMODEL_H
#define AKONADI_CONTACTSFILTERMODEL_H

#include "akonadi-contact_export.h"

#include <QtGui/QSortFilterProxyModel>

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
 * Akonadi::ContactsFilterModel *filter = new Akonadi::ContactsFilterModel;
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
//AK_REVIEW: rename to ContactsFilterProxyModel, extend API docs
class AKONADI_CONTACT_EXPORT ContactsFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new contacts filter model.
     *
     * @param parent The parent object.
     */
    explicit ContactsFilterModel( QObject *parent = 0 );

    /**
     * Destroys the contacts filter model.
     */
    ~ContactsFilterModel();

  public Q_SLOTS:
    /**
     * Sets the @p filter that is used to filter for matching contacts
     * and contact groups.
     */
    void setFilterString( const QString &filter );

  protected:
    virtual bool filterAcceptsRow( int row, const QModelIndex &parent ) const;
    virtual bool lessThan( const QModelIndex &left, const QModelIndex &right ) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
