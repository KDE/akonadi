/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>
                  2016 David Faure <faure@kde.org>s

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

#ifndef AKONADI_STATISTICSPROXYMODEL_H
#define AKONADI_STATISTICSPROXYMODEL_H

#include "akonadicore_export.h"

#include <KExtraColumnsProxyModel>

namespace Akonadi
{

/**
 * @short A proxy model that exposes collection statistics through extra columns.
 *
 * This class can be used on top of an EntityTreeModel to display extra columns
 * summarizing statistics of collections.
 *
 * @code
 *
 *   Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( ... );
 *
 *   Akonadi::StatisticsProxyModel *proxy = new Akonadi::StatisticsProxyModel();
 *   proxy->setSourceModel( model );
 *
 *   Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @author Kevin Ottens <ervin@kde.org>, now maintained by David Faure <faure@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT StatisticsProxyModel : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new statistics proxy model.
     *
     * @param parent The parent object.
     */
    explicit StatisticsProxyModel(QObject *parent = nullptr);

    /**
     * Destroys the statistics proxy model.
     */
    ~StatisticsProxyModel() override;

    /**
     * @param enable Display tooltips
     * By default, tooltips are disabled.
     */
    void setToolTipEnabled(bool enable);

    /**
     * Return true if we display tooltips, otherwise false
     */
    Q_REQUIRED_RESULT bool isToolTipEnabled() const;

    /**
     * @param enable Display extra statistics columns
     * By default, the extra columns are enabled.
     */
    void setExtraColumnsEnabled(bool enable);

    /**
     * Return true if we display extra statistics columns, otherwise false
     */
    Q_REQUIRED_RESULT bool isExtraColumnsEnabled() const;

    Q_REQUIRED_RESULT QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const override;
    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;

    Q_REQUIRED_RESULT virtual QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1,
                                  Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

    void setSourceModel(QAbstractItemModel *model) override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
