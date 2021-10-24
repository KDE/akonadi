/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

#include <QDialog>

#include <memory>

namespace Akonadi
{
class SubscriptionDialogPrivate;

/**
 * Local subscription dialog.
 */
class AKONADIWIDGETS_EXPORT SubscriptionDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Creates a new subscription dialog.
     *
     * @param parent The parent widget.
     */
    explicit SubscriptionDialog(QWidget *parent = nullptr);

    /**
     * Creates a new subscription dialog.
     *
     * @param parent The parent widget.
     * @param mimetypes The specific mimetypes
     * @since 4.6
     */
    explicit SubscriptionDialog(const QStringList &mimetypes, QWidget *parent = nullptr);

    /**
     * Destroys the subscription dialog.
     *
     * @note Don't call the destructor manually, the dialog will
     *       be destructed automatically as soon as all changes
     *       are written back to the server.
     */
    ~SubscriptionDialog() override;

    /**
     * @param showHidden shows hidden collections if set as @c true
     * @since 4.9
     */
    void showHiddenCollection(bool showHidden);

private:
    std::unique_ptr<SubscriptionDialogPrivate> const d;
};

}

