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
     */
    explicit TagSelectionDialog(QWidget *parent = nullptr);
    /*!
     */
    TagSelectionDialog(TagModel *model, QWidget *parent = nullptr);
    /*!
     */
    ~TagSelectionDialog() override;

    /*!
     */
    void setSelection(const Akonadi::Tag::List &tags);
    /*!
     */
    [[nodiscard]] Akonadi::Tag::List selection() const;

    /*!
     */
    [[nodiscard]] QDialogButtonBox *buttons() const;

Q_SIGNALS:
    /*!
     */
    void selectionChanged(const Akonadi::Tag::List &tags);

private:
    std::unique_ptr<TagSelectionDialogPrivate> const d;
};

}
