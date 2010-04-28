
#ifndef EMAILADDRESSSELECTIONPROXYMODEL_H
#define EMAILADDRESSSELECTIONPROXYMODEL_H

#include "leafextensionproxymodel_p.h"

#include "contactstreemodel.h"

namespace Akonadi {

class EmailAddressSelectionProxyModel : public Akonadi::LeafExtensionProxyModel
{
  Q_OBJECT

  public:
    enum Role
    {
      NameRole = ContactsTreeModel::DateRole + 1,
      EmailAddressRole
    };

    EmailAddressSelectionProxyModel( QObject *parent = 0 );
    ~EmailAddressSelectionProxyModel();

    QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;

  protected:
    /**
     * This method is called to retrieve the row count for the given leaf @p index.
     */
    int leafRowCount( const QModelIndex &index ) const;

    /**
     * This methid is called to retrieve the column count for the given leaf @p index.
     */
    int leafColumnCount( const QModelIndex &index ) const;

    /**
     * This methid is called to retrieve the data of the child of the given leaf @p index
     * at @p row and @p column with the given @p role.
     */
    QVariant leafData( const QModelIndex &index, int row, int column, int role = Qt::DisplayRole ) const;

  private:
};

}

#endif
