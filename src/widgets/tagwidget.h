/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
// AkonadiCore
#include "akonadi/tag.h"

#include <QLineEdit>

#include <memory>

namespace Akonadi
{
class TagWidgetPrivate;

/*!
 * A widget that shows a tag selection and provides means to edit that selection.
 *
 * \class Akonadi::TagWidget
 * \inheaderfile Akonadi/TagWidget
 * \inmodule AkonadiWidgets
 *
 * TODO A standalone dialog version that takes an item and takes care of writing back the changes would be useful.
 * \since 4.13
 */
class AKONADIWIDGETS_EXPORT TagWidget : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag widget.
     * \a parent The parent widget.
     */
    explicit TagWidget(QWidget *parent = nullptr);
    /*!
     * Destroys the tag widget.
     */
    ~TagWidget() override;

    /*!
     * Sets the selected tags.
     * \a tags The list of tags to select.
     */
    void setSelection(const Akonadi::Tag::List &tags);
    /*!
     * Returns the currently selected tags.
     * \return A list of selected tags.
     */
    [[nodiscard]] Akonadi::Tag::List selection() const;

    /*!
     * Clears all selected tags.
     */
    void clearTags();
    /*!
     * Sets whether the widget is read-only.
     * \a readOnly True to make read-only, false to allow editing.
     */
    void setReadOnly(bool readOnly);
Q_SIGNALS:
    /*!
     * Emitted when the tag selection has changed.
     * \a tags The new list of selected tags.
     */
    void selectionChanged(const Akonadi::Tag::List &tags);

private:
    void updateView();
    void editTags();
    std::unique_ptr<TagWidgetPrivate> const d;
};

}
