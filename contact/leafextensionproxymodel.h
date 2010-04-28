
#ifndef LEAFEXTENSIONPROXYMODEL_H
#define LEAFEXTENSIONPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

class LeafExtensionProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    LeafExtensionProxyModel( QObject *parent = 0 );
    ~LeafExtensionProxyModel();

    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex &index ) const;
    int rowCount( const QModelIndex &index ) const;
    int columnCount( const QModelIndex &index ) const;

    QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags( const QModelIndex &index ) const;
    bool setData( const QModelIndex &index, const QVariant &data, int role = Qt::EditRole );
    bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex buddy( const QModelIndex &index ) const;
    void fetchMore( const QModelIndex &index );

    void setSourceModel( QAbstractItemModel *sourceModel );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void sourceRowsInserted( const QModelIndex&, int, int ) )
    Q_PRIVATE_SLOT( d, void sourceRowsRemoved( const QModelIndex&, int, int ) )
    //@endcond
};

#endif
