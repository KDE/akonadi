/*
    SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

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
class AccountActivitiesAbstract;
/*!
 * \class Akonadi::ManageAccountWidget
 * \inheaderfile Akonadi/ManageAccountWidget
 * \inmodule AkonadiWidgets
 *
 * \brief The ManageAccountWidget class
 */
class AKONADIWIDGETS_EXPORT ManageAccountWidget : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new manage account widget.
     * \a parent The parent widget.
     */
    explicit ManageAccountWidget(QWidget *parent);
    /*!
     * Destroys the manage account widget.
     */
    ~ManageAccountWidget() override;

    /*!
     * Sets the description label text displayed above the account list.
     * \a text The description text (e.g., "Incoming accounts:").
     */
    void setDescriptionLabelText(const QString &text);

    /*!
     * Sets the special collection identifier for filtering.
     * \a identifier The special collection identifier.
     */
    void setSpecialCollectionIdentifier(const QString &identifier);

    /*!
     * Returns the MIME type filter for accounts.
     * \return The list of MIME types to filter by.
     */
    [[nodiscard]] QStringList mimeTypeFilter() const;
    /*!
     * Sets the MIME type filter for accounts.
     * \a mimeTypeFilter The list of MIME types to filter by.
     */
    void setMimeTypeFilter(const QStringList &mimeTypeFilter);

    /*!
     * Returns the capability filter for accounts.
     * \return The list of capabilities to filter by.
     */
    [[nodiscard]] QStringList capabilityFilter() const;
    /*!
     * Sets the capability filter for accounts.
     * \a capabilityFilter The list of capabilities to filter by.
     */
    void setCapabilityFilter(const QStringList &capabilityFilter);

    /*!
     * Returns the excluded capabilities list.
     * \return The list of excluded capabilities.
     */
    [[nodiscard]] QStringList excludeCapabilities() const;
    /*!
     * Sets the capabilities to exclude from the account list.
     * \a excludeCapabilities The list of capabilities to exclude.
     */
    void setExcludeCapabilities(const QStringList &excludeCapabilities);

    /*!
     * Sets a custom item delegate for the account list view.
     * \a delegate The custom delegate.
     */
    void setItemDelegate(QAbstractItemDelegate *delegate);

    /*!
     * Returns the item view used to display accounts.
     * \return The account view.
     */
    [[nodiscard]] QAbstractItemView *view() const;

    /*!
     */
    [[nodiscard]] QPushButton *addAccountButton() const;
    /*!
     */
    void disconnectAddAccountButton();
    /*!
     */

    [[nodiscard]] bool enablePlasmaActivities() const;
    /*!
     */
    void setEnablePlasmaActivities(bool newEnablePlasmaActivities);

    /*!
     */
    [[nodiscard]] AccountActivitiesAbstract *accountActivitiesAbstract() const;
    /*!
     */
    void setAccountActivitiesAbstract(AccountActivitiesAbstract *abstract);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public Q_SLOTS:
    /*!
     */
    void slotAddAccount();

private:
    AKONADIWIDGETS_NO_EXPORT void slotAccountSelected(const Akonadi::AgentInstance &current);
    AKONADIWIDGETS_NO_EXPORT void slotRemoveSelectedAccount();
    AKONADIWIDGETS_NO_EXPORT void slotRestartSelectedAccount();
    AKONADIWIDGETS_NO_EXPORT void slotModifySelectedAccount();
    AKONADIWIDGETS_NO_EXPORT void slotSearchAgentType(const QString &str);

private:
    std::unique_ptr<ManageAccountWidgetPrivate> const d;
};
}
