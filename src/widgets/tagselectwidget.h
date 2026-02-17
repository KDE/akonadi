/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

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
class TagSelectWidgetPrivate;

/*!
 * A widget that offers facilities to add/remove tags and provides a way to select tags.
 *
 * \class Akonadi::TagSelectWidget
 * \inheaderfile Akonadi/TagSelectWidget
 * \inmodule AkonadiWidgets
 *
 * \since 4.14.6
 */

class AKONADIWIDGETS_EXPORT TagSelectWidget : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Creates a new tag select widget.
     * \a parent The parent widget.
     */
    explicit TagSelectWidget(QWidget *parent = nullptr);
    /*!
     * Destroys the tag select widget.
     */
    ~TagSelectWidget() override;

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
     * Converts the selected tags to a string list of URLs.
     * \return A list of tag URL strings.
     */
    [[nodiscard]] QStringList tagToStringList() const;
    /*!
     * Sets the selection from a string list of tag URLs.
     * \a lst A list of tag URL strings.
     */
    void setSelectionFromStringList(const QStringList &lst);

private:
    std::unique_ptr<TagSelectWidgetPrivate> const d;
};
}
