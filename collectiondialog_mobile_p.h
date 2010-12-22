
#ifndef AKONADI_COLLECTIONDIALOG_MOBILE_P_H
#define AKONADI_COLLECTIONDIALOG_MOBILE_P_H

#include "collectiondialog.h"

class KJob;
class QDeclarativeView;
class QSortFilterProxyModel;

namespace Akonadi {

class AsyncSelectionHandler;
class EntityRightsFilterModel;
class EntityTreeModel;
class ChangeRecorder;
class CollectionFilterProxyModel;

class CollectionDialog::Private : public QObject
{
  Q_OBJECT

  Q_PROPERTY( QString descriptionText READ descriptionText NOTIFY descriptionTextChanged )
  Q_PROPERTY( bool okButtonEnabled READ okButtonEnabled NOTIFY buttonStatusChanged )
  Q_PROPERTY( bool cancelButtonEnabled READ cancelButtonEnabled NOTIFY buttonStatusChanged )
  Q_PROPERTY( bool createButtonEnabled READ createButtonEnabled NOTIFY buttonStatusChanged )
  Q_PROPERTY( bool createButtonVisible READ createButtonVisible NOTIFY buttonStatusChanged )

  public:
    Private( QAbstractItemModel *customModel, CollectionDialog *parent, CollectionDialogOptions options );

    ~Private();

    void slotSelectionChanged();
    void slotAddChildCollection();
    void slotCollectionCreationResult( KJob* job );
    void slotCollectionAvailable( const QModelIndex &index );
    bool canCreateCollection( const Akonadi::Collection &parentCollection ) const;
    void changeCollectionDialogOptions( CollectionDialogOptions options );

    void setDescriptionText( const QString &text );
    QString descriptionText() const;

    bool okButtonEnabled() const;
    bool cancelButtonEnabled() const;
    bool createButtonEnabled() const;
    bool createButtonVisible() const;

  public Q_SLOTS:
    void okClicked();
    void cancelClicked();
    void createClicked();
    void setCurrentIndex( int index );
    void setFilterText( const QString &text );
    void selectionChanged( const QItemSelection&, const QItemSelection& );

  Q_SIGNALS:
    void descriptionTextChanged();
    void buttonStatusChanged();
    void selectionChanged( int row );

  public:
    CollectionDialog *mParent;
    ChangeRecorder *mMonitor;
    EntityTreeModel *mModel;
    CollectionFilterProxyModel *mMimeTypeFilterModel;
    EntityRightsFilterModel *mRightsFilterModel;
    AsyncSelectionHandler *mSelectionHandler;
    QItemSelectionModel *mSelectionModel;
    QSortFilterProxyModel *mFilterModel;

    QAbstractItemView::SelectionMode mSelectionMode;
    QDeclarativeView *mView;
    bool mAllowToCreateNewChildCollection;
    QString mDescriptionText;
    bool mOkButtonEnabled;
    bool mCancelButtonEnabled;
    bool mCreateButtonEnabled;
};

}

#endif
