/*
    SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MANAGEACCOUNTWIDGET_H
#define MANAGEACCOUNTWIDGET_H

#include "akonadiwidgets_export.h"
#include <QWidget>

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
    void slotSearchAgentType(const QString &str);
    QScopedPointer<ManageAccountWidgetPrivate> const d;
};
}

#endif // MANAGEACCOUNTWIDGET_H
