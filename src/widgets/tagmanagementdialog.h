/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
// AkonadiCore
#include "akonadi/tag.h"

#include <QDialog>

#include <memory>

class QDialogButtonBox;
namespace Akonadi
{
class TagManagementDialogPrivate;

/*!
 * \class Akonadi::TagManagementDialog
 * \inheaderfile Akonadi/TagManagementDialog
 * \inmodule AkonadiWidgets
 *
 * A dialog to manage tags.
 *
 * \since 4.13
 */
class AKONADIWIDGETS_EXPORT TagManagementDialog : public QDialog
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag management dialog.
     * \a parent The parent widget.
     */
    explicit TagManagementDialog(QWidget *parent = nullptr);
    /*!
     * Destroys the tag management dialog.
     */
    ~TagManagementDialog() override;

    /*!
     * Returns the button box of the dialog.
     * \return The dialog button box.
     */
    [[nodiscard]] QDialogButtonBox *buttons() const;

private:
    std::unique_ptr<TagManagementDialogPrivate> const d;
};

}
