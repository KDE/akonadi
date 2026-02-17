/*
  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
  SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

// AkonadiCore
#include "akonadi/tag.h"

#include <QComboBox>

#include <memory>

namespace Akonadi
{
class TagSelectionComboBoxPrivate;

/*!
 * \class Akonadi::TagSelectionComboBox
 * \inheaderfile Akonadi/TagSelectionComboBox
 * \inmodule AkonadiWidgets
 *
 * \brief The TagSelectionCombo class
 */
class AKONADIWIDGETS_EXPORT TagSelectionComboBox : public QComboBox
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag selection combobox.
     * \a parent The parent widget.
     */
    explicit TagSelectionComboBox(QWidget *parent = nullptr);
    /*!
     * Destroys the tag selection combobox.
     */
    ~TagSelectionComboBox() override;

    /*!
     * Sets whether tags can be selected with checkboxes.
     * \a checkable True to enable checkbox selection, false otherwise.
     */
    void setCheckable(bool checkable);
    /*!
     * Returns whether tag selection with checkboxes is enabled.
     * \return True if checkbox selection is enabled, false otherwise.
     */
    [[nodiscard]] bool checkable() const;

    /*!
     * Returns the list of currently selected tags.
     * \return A list of selected Tag objects.
     */
    [[nodiscard]] Tag::List selection() const;
    /*!
     * Returns the names of currently selected tags.
     * \return A list of tag name strings.
     */
    [[nodiscard]] QStringList selectionNames() const;
    /*!
     * Sets the selected tags.
     * \a selection The list of tags to select.
     */
    void setSelection(const Tag::List &selection);
    /*!
     * Sets the selected tags by name.
     * \a selection The list of tag name strings to select.
     */
    void setSelection(const QStringList &selection);

    /*!
     * Hides the combobox popup.
     */
    void hidePopup() override;

protected:
    /*!
     * Handles key press events.
     * \a event The key event to handle.
     */
    void keyPressEvent(QKeyEvent *event) override;
    /*!
     * Filters events for the given object.
     * \a receiver The object whose events are being filtered.
     * \a event The event to filter.
     * \return True if the event should be filtered, false otherwise.
     */
    bool eventFilter(QObject *receiver, QEvent *event) override;

Q_SIGNALS:
    /*!
     * Emitted when the tag selection has changed.
     * \a selection The new list of selected tags.
     */
    void selectionChanged(const Akonadi::Tag::List &selection);

private:
    std::unique_ptr<TagSelectionComboBoxPrivate> const d;
};

} // namespace
