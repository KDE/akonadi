/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>
                  2016 David Faure <faure@kde.org>s

    SPDX-License-Identifier: LGPL-2.0-or-later
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

    Q_REQUIRED_RESULT virtual QModelIndexList match(const QModelIndex &start,
                                                    int role,
                                                    const QVariant &value,
                                                    int hits = 1,
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
