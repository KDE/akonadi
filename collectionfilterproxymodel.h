/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef AKONADI_COLLECTIONPROXYFILTERMODEL_H
#define AKONADI_COLLECTIONPROXYFILTERMODEL_H

#include "libakonadi_export.h"
#include <QtGui/QSortFilterProxyModel>

class QString;
class QModelIndex;

namespace Akonadi {

class CollectionModel;

/**
 * Proxy model to filter collections : only shows collections containing a given types.
 * For instance, a mail application will useful addMimeType("message/rfc822") to only show
 * folders containing mail.
 *
 * Example use:
 * <code>
 * m_folderModel = new Akonadi::CollectionModel(this);
 * m_folderProxyModel = new Akonadi::CollectionFilterProxyModel();
 * m_folderProxyModel->addMimeType("message/rfc822");
 * m_folderProxyModel->setSourceModel(m_folderModel);
 * </code>
*/
class AKONADI_EXPORT CollectionFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Create a new CollectionProxyFilterModel
     * @param parent The parent object
     */
    CollectionFilterProxyModel( QObject *parent = 0 );

    /**
     * Destroy the model
     **/
    virtual ~CollectionFilterProxyModel();

    /**
     * Add types to be shown by the filter
     * @param typeList A list of mimetypes to be shown
     */
    void addMimeTypes( const QStringList &typeList );

    /**
     * Convenience method for the previous one
     * @param type A type to show
     */
    void addMimeType( const QString &type );

    /**
     * @return The list of supported mimetypes
     */
    QStringList mimeTypes() const;

  protected:
    /**
     * Reimplemented to only accept rows (collections) which are able to contain the filter mimetypes.
     */
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const; 

  private:
    class Private;
    Private* const d;
};

}

#endif
