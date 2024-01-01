/*
    SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include <QWidget>

#include <memory>

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

    [[nodiscard]] QStringList mimeTypeFilter() const;
    void setMimeTypeFilter(const QStringList &mimeTypeFilter);

    [[nodiscard]] QStringList capabilityFilter() const;
    void setCapabilityFilter(const QStringList &capabilityFilter);

    [[nodiscard]] QStringList excludeCapabilities() const;
    void setExcludeCapabilities(const QStringList &excludeCapabilities);

    void setItemDelegate(QAbstractItemDelegate *delegate);

    [[nodiscard]] QAbstractItemView *view() const;

    [[nodiscard]] QPushButton *addAccountButton() const;
    void disconnectAddAccountButton();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public Q_SLOTS:
    void slotAddAccount();

private Q_SLOTS:
    AKONADIWIDGETS_NO_EXPORT void slotAccountSelected(const Akonadi::AgentInstance &current);
    AKONADIWIDGETS_NO_EXPORT void slotRemoveSelectedAccount();
    AKONADIWIDGETS_NO_EXPORT void slotRestartSelectedAccount();
    AKONADIWIDGETS_NO_EXPORT void slotModifySelectedAccount();

private:
    AKONADIWIDGETS_NO_EXPORT void slotSearchAgentType(const QString &str);

private:
    std::unique_ptr<ManageAccountWidgetPrivate> const d;
};
}
