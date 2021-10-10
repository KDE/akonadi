/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
// AkonadiCore
#include <akonadi/tag.h>

#include <QLineEdit>

#include <memory>

namespace Akonadi
{
/**
 * A widget that shows a tag selection and provides means to edit that selection.
 *
 * TODO A standalone dialog version that takes an item and takes care of writing back the changes would be useful.
 * @since 4.13
 */
class AKONADIWIDGETS_EXPORT TagWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TagWidget(QWidget *parent = nullptr);
    ~TagWidget();

    void setSelection(const Akonadi::Tag::List &tags);
    Q_REQUIRED_RESULT Akonadi::Tag::List selection() const;

    void clearTags();
    void setReadOnly(bool readOnly);
Q_SIGNALS:
    void selectionChanged(const Akonadi::Tag::List &tags);

private Q_SLOTS:
    void editTags();
    void updateView();

private:
    class Private;
    std::unique_ptr<Private> const d;
};

}

