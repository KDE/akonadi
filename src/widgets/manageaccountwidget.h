/*
    Copyright (c) 2014-2020 Laurent Montel <montel@kde.org>

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

class QAbstractItemDelegate;
class QAbstractItemView;
class QPushButton;

namespace Akonadi
{
class AgentInstance;
class ManageAccountWidgetPrivate;

class AKONADIWIDGETS_EXPORT ManageAccountWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ManageAccountWidget(QWidget *parent);
    ~ManageAccountWidget() override;

    /**
     * Sets the text of the label above the list of accounts.
     * Example: "Incoming accounts:" in an email client, or "Calendars:" in an organizer.
     */
    void setDescriptionLabelText(const QString &text);

    void setSpecialCollectionIdentifier(const QString &identifier);

    Q_REQUIRED_RESULT QStringList mimeTypeFilter() const;
    void setMimeTypeFilter(const QStringList &mimeTypeFilter);

    Q_REQUIRED_RESULT QStringList capabilityFilter() const;
    void setCapabilityFilter(const QStringList &capabilityFilter);

    Q_REQUIRED_RESULT QStringList excludeCapabilities() const;
    void setExcludeCapabilities(const QStringList &excludeCapabilities);

    void setItemDelegate(QAbstractItemDelegate *delegate);

    Q_REQUIRED_RESULT QAbstractItemView *view() const;

    Q_REQUIRED_RESULT QPushButton *addAccountButton() const;
    void disconnectAddAccountButton();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public Q_SLOTS:
    void slotAddAccount();

private Q_SLOTS:
    void slotAccountSelected(const Akonadi::AgentInstance &current);
    void slotRemoveSelectedAccount();
    void slotRestartSelectedAccount();
    void slotModifySelectedAccount();

private:
    QScopedPointer<ManageAccountWidgetPrivate> const d;
};
}

#endif // MANAGEACCOUNTWIDGET_H
