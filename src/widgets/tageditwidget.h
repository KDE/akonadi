/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
// AkonadiCore
#include "akonadi/tag.h"

#include <QWidget>

#include <memory>

namespace Akonadi
{
class TagModel;
class TagEditWidgetPrivate;

/*!
 * \class Akonadi::TagEditWidget
 * \inheaderfile Akonadi/TagEditWidget
 * \inmodule AkonadiWidgets
 *
 * A widget that offers facilities to add/remove tags and optionally provides a way to select tags.
 *
 * \since 4.13
 */
class AKONADIWIDGETS_EXPORT TagEditWidget : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag edit widget.
     * \a parent The parent widget.
     */
    explicit TagEditWidget(QWidget *parent = nullptr);
    /*!
     * Creates a new tag edit widget with the given tag model.
     * \a model The tag model to use.
     * \a parent The parent widget.
     * \a enableSelection Whether to enable tag selection.
     */
    explicit TagEditWidget(Akonadi::TagModel *model, QWidget *parent = nullptr, bool enableSelection = false);
    /*!
     * Destroys the tag edit widget.
     */
    ~TagEditWidget() override;

    /*!
     * Sets the tag model for this widget.
     * \a model The tag model to use.
     */
    void setModel(Akonadi::TagModel *model);
    /*!
     * Returns the currently used tag model.
     * \return The tag model.
     */
    Akonadi::TagModel *model() const;

    /*!
     * Sets whether tag selection is enabled.
     * \a enabled True to enable selection, false otherwise.
     */
    void setSelectionEnabled(bool enabled);
    /*!
     * Returns whether tag selection is enabled.
     * \return True if selection is enabled, false otherwise.
     */
    [[nodiscard]] bool selectionEnabled() const;

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

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::unique_ptr<TagEditWidgetPrivate> const d;
};

}
