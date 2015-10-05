/*
    Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#ifndef MANAGEACCOUNTWIDGET_H
#define MANAGEACCOUNTWIDGET_H

#include <QWidget>
#include "akonadiwidgets_export.h"
namespace Ui
{
class ManageAccountWidget;
}
namespace Akonadi
{
class AgentInstance;
}
class QAbstractItemDelegate;
class QAbstractItemView;
namespace Akonadi
{
class ManageAccountWidgetPrivate;
class AKONADIWIDGETS_EXPORT ManageAccountWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ManageAccountWidget(QWidget *parent);
    ~ManageAccountWidget();

    void setSpecialCollectionIdentifier(const QString &identifier);

    QStringList mimeTypeFilter() const;
    void setMimeTypeFilter(const QStringList &mimeTypeFilter);

    QStringList capabilityFilter() const;
    void setCapabilityFilter(const QStringList &capabilityFilter);

    QStringList excludeCapabilities() const;
    void setExcludeCapabilities(const QStringList &excludeCapabilities);

    void setItemDelegate(QAbstractItemDelegate *delegate);

    QAbstractItemView *view() const;
protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
private Q_SLOTS:
    void slotAccountSelected(const Akonadi::AgentInstance &current);
    void slotRemoveSelectedAccount();
    void slotRestartSelectedAccount();
    void slotModifySelectedAccount();
    void slotAddAccount();
private:
    ManageAccountWidgetPrivate *const d;
};
}

#endif // MANAGEACCOUNTWIDGET_H
