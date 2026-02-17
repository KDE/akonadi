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
class TagModel;
class TagSelectionDialogPrivate;

/*!
 * \class Akonadi::TagSelectionDialog
 * \inheaderfile Akonadi/TagSelectionDialog
 * \inmodule AkonadiWidgets
 *
 * A widget that shows a tag selection and provides means to edit that selection.
 *
 * TODO A standalone dialog version that takes an item and takes care of writing back the changes would be useful.
 * \since 4.13
 */
class AKONADIWIDGETS_EXPORT TagSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag selection dialog.
     * \a parent The parent widget.
     */
    explicit TagSelectionDialog(QWidget *parent = nullptr);
    /*!
     * Creates a new tag selection dialog with a custom tag model.
     * \a model The tag model to use.
     * \a parent The parent widget.
     */
    TagSelectionDialog(TagModel *model, QWidget *parent = nullptr);
    /*!
     * Destroys the tag selection dialog.
     */
    ~TagSelectionDialog() override;

    /*!
     * Sets the selected tags in the dialog.
     * \a tags The list of tags to select.
     */
    void setSelection(const Akonadi::Tag::List &tags);
    /*!
     * Returns the tags selected in the dialog.
     * \return A list of selected tags.
     */
    [[nodiscard]] Akonadi::Tag::List selection() const;

    /*!
     * Returns the button box of the dialog.
     * \return The dialog button box.
     */
    [[nodiscard]] QDialogButtonBox *buttons() const;

Q_SIGNALS:
    /*!
     * Emitted when the tag selection has changed.
     * \a tags The new list of selected tags.
     */
    void selectionChanged(const Akonadi::Tag::List &tags);

private:
    std::unique_ptr<TagSelectionDialogPrivate> const d;
};

}
