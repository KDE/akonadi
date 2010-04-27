
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

  public Q_SLOTS:
/*
    void sourceRowsAboutToBeInserted(const QModelIndex &, int, int);
    void sourceRowsInserted(const QModelIndex &, int, int);
*/
    void sourceRowsAboutToBeRemoved(const QModelIndex &, int, int);
    void sourceRowsRemoved(const QModelIndex &, int, int);
/*
    void sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void sourceModelAboutToBeReset();
    void sourceModelReset();
    void sourceDataChanged(const QModelIndex &, const QModelIndex & );
    void sourceHeaderDataChanged( Qt::Orientation, int first, int last );
    void sourceLayoutAboutToBeChanged();
    void sourceLayoutChanged();
    void sourceModelDestroyed();
*/
  private:
    mutable QMap<qint64, QModelIndex> mParentIndexes;
    mutable QSet<QModelIndex> mOwnIndexes;
    mutable qint64 mUniqueKeyCounter;
};

#endif
