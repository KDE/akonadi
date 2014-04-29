/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>


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

#include "statisticsproxymodel.h"
#include <QItemSelection>

#include <entitytreemodel.h>
#include <collectionutils.h>

#include <collectionquotaattribute.h>
#include <collectionstatistics.h>
#include <entitydisplayattribute.h>

#include <qdebug.h>
#include <kiconloader.h>
#include <klocalizedstring.h>
#include <kio/global.h>

#include <QApplication>
#include <QPalette>
#include <KIcon>
using namespace Akonadi;

/**
 * @internal
 */
class StatisticsProxyModel::Private
{
public:
    Private( StatisticsProxyModel *parent )
        : mParent( parent ), mToolTipEnabled( false ), mExtraColumnsEnabled( true )
    {
    }

    void getCountRecursive( const QModelIndex &index, qint64 &totalSize ) const
    {
        Collection collection = qvariant_cast<Collection>( index.data( EntityTreeModel::CollectionRole ) );
        // Do not assert on invalid collections, since a collection may be deleted
        // in the meantime and deleted collections are invalid.
        if ( collection.isValid() ) {
            CollectionStatistics statistics = collection.statistics();
            totalSize += qMax( 0LL, statistics.size() );
            if ( index.model()->hasChildren( index ) ) {
                const int rowCount = index.model()->rowCount( index );
                for ( int row = 0; row < rowCount; row++ ) {
                    static const int column = 0;
                    getCountRecursive( index.model()->index( row, column, index ),  totalSize );
                }
            }
        }
    }

    int sourceColumnCount() const
    {
        return mParent->sourceModel()->columnCount();
    }

    QModelIndex sourceIndexAtFirstColumn(const QModelIndex& proxyIndex) const;

    QString toolTipForCollection( const QModelIndex &index, const Collection &collection )
    {
        QString bckColor = QApplication::palette().color( QPalette::ToolTipBase ).name();
        QString txtColor = QApplication::palette().color( QPalette::ToolTipText ).name();

        QString tip = QString::fromLatin1(
                    "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n"
                    );
        const QString textDirection =  ( QApplication::layoutDirection() == Qt::LeftToRight ) ? QLatin1String( "left" ) : QLatin1String( "right" );
        tip += QString::fromLatin1(
                    "  <tr>\n"
                    "    <td bgcolor=\"%1\" colspan=\"2\" align=\"%4\" valign=\"middle\">\n"
                    "      <div style=\"color: %2; font-weight: bold;\">\n"
                    "      %3\n"
                    "      </div>\n"
                    "    </td>\n"
                    "  </tr>\n"
                    ).arg( txtColor ).arg( bckColor ).arg( index.data( Qt::DisplayRole ).toString() ).arg( textDirection );


        tip += QString::fromLatin1(
                    "  <tr>\n"
                    "    <td align=\"%1\" valign=\"top\">\n"
                    ).arg( textDirection );

        QString tipInfo;
        tipInfo += QString::fromLatin1(
                    "      <strong>%1</strong>: %2<br>\n"
                    "      <strong>%3</strong>: %4<br><br>\n"
                    ).arg( i18n( "Total Messages" ) ).arg( collection.statistics().count() )
                .arg( i18n( "Unread Messages" ) ).arg( collection.statistics().unreadCount() );

        if ( collection.hasAttribute<CollectionQuotaAttribute>() ) {
            CollectionQuotaAttribute *quota = collection.attribute<CollectionQuotaAttribute>();
            if ( quota->currentValue() > -1 && quota->maximumValue() > 0 ) {
                qreal percentage = ( 100.0 * quota->currentValue() ) / quota->maximumValue();

                if ( qAbs( percentage ) >= 0.01 ) {
                    QString percentStr = QString::number( percentage, 'f', 2 );
                    tipInfo += QString::fromLatin1(
                                "      <strong>%1</strong>: %2%<br>\n"
                                ).arg( i18n( "Quota" ) ).arg( percentStr );
                }
            }
        }

        qint64 currentFolderSize( collection.statistics().size() );
        tipInfo += QString::fromLatin1(
                    "      <strong>%1</strong>: %2<br>\n"
                    ).arg( i18n( "Storage Size" ) ).arg( KIO::convertSize( (KIO::filesize_t)( currentFolderSize ) ) );


        qint64 totalSize = 0;
        getCountRecursive( index, totalSize );
        totalSize -= currentFolderSize;
        if (totalSize > 0 ) {
            tipInfo += QString::fromLatin1(
                        "<strong>%1</strong>: %2<br>"
                        ).arg( i18n("Subfolder Storage Size") ).arg( KIO::convertSize( (KIO::filesize_t)( totalSize ) ) );
        }

        QString iconName = CollectionUtils::defaultIconName( collection );
        if ( collection.hasAttribute<EntityDisplayAttribute>() &&
             !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty() ) {
            if ( !collection.attribute<EntityDisplayAttribute>()->activeIconName().isEmpty() && collection.statistics().unreadCount()> 0) {
                iconName = collection.attribute<EntityDisplayAttribute>()->activeIconName();
            }
            else
                iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
        }


        int iconSizes[] = { 32, 22 };
        int icon_size_found = 32;

        QString iconPath;

        for ( int i = 0; i < 2; ++i ) {
            iconPath = KIconLoader::global()->iconPath( iconName, -iconSizes[ i ], true );
            if ( !iconPath.isEmpty() ) {
                icon_size_found = iconSizes[ i ];
                break;
            }
        }

        if ( iconPath.isEmpty() ) {
            iconPath = KIconLoader::global()->iconPath( QLatin1String( "folder" ), -32, false );
        }

        QString tipIcon = QString::fromLatin1(
                    "      <table border=\"0\"><tr><td width=\"32\" height=\"32\" align=\"center\" valign=\"middle\">\n"
                    "      <img src=\"%1\" width=\"%2\" height=\"32\">\n"
                    "      </td></tr></table>\n"
                    "    </td>\n"
                    ).arg( iconPath ).arg( icon_size_found ) ;

        if ( QApplication::layoutDirection() == Qt::LeftToRight )
        {
            tip += tipInfo + QString::fromLatin1( "</td><td align=\"%3\" valign=\"top\">" ).arg( textDirection ) + tipIcon;
        }
        else
        {
            tip += tipIcon + QString::fromLatin1( "</td><td align=\"%3\" valign=\"top\">" ).arg( textDirection ) + tipInfo;
        }


        tip += QString::fromLatin1(
                    "  </tr>" \
                    "</table>"
                    );

        return tip;
    }

    void proxyDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void sourceLayoutAboutToBeChanged();
    void sourceLayoutChanged();

    QVector<QModelIndex> m_proxyIndexes;
    QVector<QPersistentModelIndex> m_persistentSourceFirstColumn;

    StatisticsProxyModel *mParent;

    bool mToolTipEnabled;
    bool mExtraColumnsEnabled;
};

void StatisticsProxyModel::Private::proxyDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if ( mExtraColumnsEnabled )
    {
        // Ugly hack.
        // The proper solution is a KExtraColumnsProxyModel, but this will do for now.
        QModelIndex parent = topLeft.parent();
        int parentColumnCount = mParent->columnCount( parent );
        QModelIndex extraTopLeft = mParent->index( topLeft.row(), parentColumnCount - 1 - 3 , parent );
        QModelIndex extraBottomRight = mParent->index( bottomRight.row(), parentColumnCount -1, parent );
        mParent->disconnect( mParent, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                             mParent, SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
        emit mParent->dataChanged( extraTopLeft, extraBottomRight );

        // We get this signal when the statistics of a row changes.
        // However, we need to emit data changed for the statistics of all ancestor rows too
        // so that recursive totals can be updated.
        while ( parent.isValid() )
        {
            emit mParent->dataChanged( parent.sibling( parent.row(), parentColumnCount - 1 - 3 ),
                                       parent.sibling( parent.row(), parentColumnCount - 1 ) );
            parent = parent.parent();
            parentColumnCount = mParent->columnCount( parent );
        }
        mParent->connect( mParent, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                          SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
    }
}

void StatisticsProxyModel::Private::sourceLayoutAboutToBeChanged()
{
    // QIdentityProxyModel took care of the first columnCount() columns
    // We have to take care of the extra columns (by storing persistent indexes in column 0,
    // waiting for the source to update them, and then looking at where they ended up)
    QModelIndexList persistent = mParent->persistentIndexList();
    const int columnCount = mParent->sourceModel()->columnCount();
    foreach( const QModelIndex &proxyPersistentIndex, persistent ) {
        if ( proxyPersistentIndex.column() >= columnCount ) {
            m_proxyIndexes << proxyPersistentIndex;
            m_persistentSourceFirstColumn << QPersistentModelIndex( sourceIndexAtFirstColumn( proxyPersistentIndex ) );
        }
    }
}

void StatisticsProxyModel::Private::sourceLayoutChanged()
{
    QModelIndexList oldList;
    QModelIndexList newList;

    for( int i = 0; i < m_proxyIndexes.size(); ++i ) {
        const QModelIndex oldProxyIndex = m_proxyIndexes.at( i );
        const QModelIndex proxyIndexFirstCol = mParent->mapFromSource( m_persistentSourceFirstColumn.at( i ) );
        const QModelIndex newProxyIndex = proxyIndexFirstCol.sibling( proxyIndexFirstCol.row(), oldProxyIndex.column() );
        if ( newProxyIndex != oldProxyIndex ) {
            oldList.append( oldProxyIndex );
            newList.append( newProxyIndex );
        }
    }
    mParent->changePersistentIndexList( oldList, newList );
    m_persistentSourceFirstColumn.clear();
    m_proxyIndexes.clear();
}

void StatisticsProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    // Order is important here. sourceLayoutChanged must be called *before* any downstreams react
    // to the layoutChanged so that it can have the QPersistentModelIndexes uptodate in time.
    disconnect(this, SIGNAL(layoutChanged()), this, SLOT(sourceLayoutChanged()));
    connect(this, SIGNAL(layoutChanged()), SLOT(sourceLayoutChanged()));
    QIdentityProxyModel::setSourceModel(sourceModel);
    // This one should come *after* any downstream handlers of layoutAboutToBeChanged.
    // The connectNotify stuff below ensures that it remains the last one.
    disconnect(this, SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
    connect(this, SIGNAL(layoutAboutToBeChanged()), SLOT(sourceLayoutAboutToBeChanged()));
}

void StatisticsProxyModel::connectNotify(const char *signal)
{
    static bool ignore = false;
//QT5
#if 0
    if (ignore || QLatin1String(signal) == SIGNAL(layoutAboutToBeChanged()))
        return QIdentityProxyModel::connectNotify(signal);
#endif
    ignore = true;
    disconnect(this, SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
    connect(this, SIGNAL(layoutAboutToBeChanged()), SLOT(sourceLayoutAboutToBeChanged()));
    ignore = false;
//QT5
#if 0
    QIdentityProxyModel::connectNotify(signal);
#endif
}


StatisticsProxyModel::StatisticsProxyModel( QObject *parent )
    : QIdentityProxyModel( parent ),
      d( new Private( this ) )
{
    connect( this, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             SLOT(proxyDataChanged(QModelIndex,QModelIndex)) );
}

StatisticsProxyModel::~StatisticsProxyModel()
{
    delete d;
}

void StatisticsProxyModel::setToolTipEnabled( bool enable )
{
    d->mToolTipEnabled = enable;
}

bool StatisticsProxyModel::isToolTipEnabled() const
{
    return d->mToolTipEnabled;
}

void StatisticsProxyModel::setExtraColumnsEnabled( bool enable )
{
    d->mExtraColumnsEnabled = enable;
}

bool StatisticsProxyModel::isExtraColumnsEnabled() const
{
    return d->mExtraColumnsEnabled;
}

QModelIndex StatisticsProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();


    int sourceColumn = column;
    if ( column >= d->sourceColumnCount() ) {
        sourceColumn = 0;
    }

    QModelIndex i = QIdentityProxyModel::index( row, sourceColumn, parent );
    return createIndex( i.row(), column, i.internalPointer() );
}

struct SourceModelIndex
{
    SourceModelIndex(int _r, int _c, void *_p, QAbstractItemModel *_m)
        : r(_r), c(_c), p(_p), m(_m) {}

    operator QModelIndex() { return reinterpret_cast<QModelIndex&>(*this); }

    int r, c;
    void *p;
    const QAbstractItemModel *m;
};

QModelIndex StatisticsProxyModel::Private::sourceIndexAtFirstColumn(const QModelIndex& proxyIndex) const
{
    // We rely on the fact that the internal pointer is the same for column 0 and for the extra columns
    return SourceModelIndex(proxyIndex.row(), 0, proxyIndex.internalPointer(), mParent->sourceModel());
}

QModelIndex StatisticsProxyModel::parent(const QModelIndex& child) const
{
    if (!sourceModel())
        return QModelIndex();

    Q_ASSERT(child.isValid() ? child.model() == this : true);
    if (child.column() >= d->sourceColumnCount()) {
        // We need to get hold of the source index at column 0. But we can't do that
        // via the proxy index at column 0, because sibling() or index() needs the
        // parent index, and that's *exactly* what we're trying to determine here.
        // So the only way is to create a source index ourselves.
        const QModelIndex sourceIndex = d->sourceIndexAtFirstColumn(child);
        const QModelIndex sourceParent = sourceIndex.parent();
        //qDebug() << "parent of" << child.data() << "is" << sourceParent.data();
        return mapFromSource(sourceParent);
    } else {
        return QIdentityProxyModel::parent(child);
    }
}

QVariant StatisticsProxyModel::data( const QModelIndex & index, int role) const
{
    if (!sourceModel())
        return QVariant();

    const int sourceColumnCount = d->sourceColumnCount();

    if ( role == Qt::DisplayRole && index.column() >= sourceColumnCount ) {
        const QModelIndex sourceIndex = d->sourceIndexAtFirstColumn( index );
        Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

        if ( collection.isValid() && collection.statistics().count()>=0 ) {
            if ( index.column() == sourceColumnCount + 2 ) {
                return KIO::convertSize( (KIO::filesize_t)( collection.statistics().size() ) );
            } else if ( index.column() == sourceColumnCount + 1 ) {
                return collection.statistics().count();
            } else if ( index.column() == sourceColumnCount ) {
                if ( collection.statistics().unreadCount() > 0 ) {
                    return collection.statistics().unreadCount();
                } else {
                    return QString();
                }
            } else {
                qWarning() << "We shouldn't get there for a column which is not total, unread or size.";
                return QVariant();
            }
        }

    } else if ( role == Qt::TextAlignmentRole && index.column() >= sourceColumnCount ) {
        return Qt::AlignRight;

    } else if ( role == Qt::ToolTipRole && d->mToolTipEnabled ) {
        const QModelIndex sourceIndex = d->sourceIndexAtFirstColumn( index );
        Collection collection
                = sourceModel()->data( sourceIndex,
                                       EntityTreeModel::CollectionRole ).value<Collection>();

        if ( collection.isValid() ) {
            const QModelIndex sourceIndex = d->sourceIndexAtFirstColumn( index );
            return d->toolTipForCollection( sourceIndex, collection );
        }

    } else if ( role == Qt::DecorationRole && index.column() == 0 ) {
        const QModelIndex sourceIndex = mapToSource( index );
        Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();
        if ( collection.isValid() )
            return KIcon( CollectionUtils::displayIconName( collection ) );
        else
            return QVariant();
    }

    if ( index.column() >= sourceColumnCount )
        return QVariant();

    return QAbstractProxyModel::data( index, role );
}

QVariant StatisticsProxyModel::headerData( int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        if ( section == d->sourceColumnCount() + 2 ) {
            return i18nc( "collection size", "Size" );
        } else if ( section == d->sourceColumnCount() + 1 ) {
            return i18nc( "number of entities in the collection", "Total" );
        } else if ( section == d->sourceColumnCount() ) {
            return i18nc( "number of unread entities in the collection", "Unread" );
        }
    }

    if ( orientation == Qt::Horizontal && section >= d->sourceColumnCount() ) {
        return QVariant();
    }

    return QIdentityProxyModel::headerData( section, orientation, role );
}

Qt::ItemFlags StatisticsProxyModel::flags( const QModelIndex & index ) const
{
    if ( index.column() >= d->sourceColumnCount() ) {
        return QIdentityProxyModel::flags( index.sibling( index.row(), 0 ) )
                & ( Qt::ItemIsSelectable | Qt::ItemIsDragEnabled // Allowed flags
                    | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled );
    }

    return QIdentityProxyModel::flags( index );
}

int StatisticsProxyModel::columnCount( const QModelIndex & /*parent*/ ) const
{
    if ( sourceModel()==0 ) {
        return 0;
    } else {
        return d->sourceColumnCount()
                + ( d->mExtraColumnsEnabled ? 3 : 0 );
    }
}

QModelIndexList StatisticsProxyModel::match( const QModelIndex& start, int role, const QVariant& value,
                                             int hits, Qt::MatchFlags flags ) const
{
    if ( role < Qt::UserRole )
        return QIdentityProxyModel::match( start, role, value, hits, flags );

    QModelIndexList list;
    QModelIndex proxyIndex;
    foreach ( const QModelIndex &idx, sourceModel()->match( mapToSource( start ), role, value, hits, flags ) ) {
        proxyIndex = mapFromSource( idx );
        if ( proxyIndex.isValid() )
            list << proxyIndex;
    }

    return list;
}

QModelIndex StatisticsProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();
    Q_ASSERT(sourceIndex.model() == sourceModel());
    Q_ASSERT(sourceIndex.column() < d->sourceColumnCount());
    return QIdentityProxyModel::mapFromSource(sourceIndex);
}

QModelIndex StatisticsProxyModel::mapToSource(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();
    Q_ASSERT(index.model() == this);
    if (index.column() >= d->sourceColumnCount() ) {
        return QModelIndex();
    }
    return QIdentityProxyModel::mapToSource(index);
}

QModelIndex StatisticsProxyModel::buddy(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QItemSelection StatisticsProxyModel::mapSelectionToSource(const QItemSelection& selection) const
{
    QItemSelection sourceSelection;

    if (!sourceModel())
        return sourceSelection;

    // mapToSource will give invalid index for our additional columns, so truncate the selection
    // to the columns known by the source model
    const int sourceColumnCount = d->sourceColumnCount();
    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == this);
        QModelIndex topLeft = it->topLeft();
        Q_ASSERT(topLeft.isValid());
        Q_ASSERT(topLeft.model() == this);
        topLeft = topLeft.sibling(topLeft.row(), 0);
        QModelIndex bottomRight = it->bottomRight();
        Q_ASSERT(bottomRight.isValid());
        Q_ASSERT(bottomRight.model() == this);
        if (bottomRight.column() >= sourceColumnCount)
            bottomRight = bottomRight.sibling(bottomRight.row(), sourceColumnCount-1);
        // This can lead to duplicate source indexes, so use merge().
        const QItemSelectionRange range(mapToSource(topLeft), mapToSource(bottomRight));
        QItemSelection newSelection; newSelection << range;
        sourceSelection.merge(newSelection, QItemSelectionModel::Select);
    }

    return sourceSelection;
}

#include "moc_statisticsproxymodel.cpp"

