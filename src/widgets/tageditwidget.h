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
     */
    explicit TagEditWidget(QWidget *parent = nullptr);
    /*!
     */
    explicit TagEditWidget(Akonadi::TagModel *model, QWidget *parent = nullptr, bool enableSelection = false);
    /*!
     */
    ~TagEditWidget() override;

    /*!
     */
    void setModel(Akonadi::TagModel *model);
    /*!
     */
    Akonadi::TagModel *model() const;

    /*!
     */
    void setSelectionEnabled(bool enabled);
    /*!
     */
    [[nodiscard]] bool selectionEnabled() const;

    /*!
     */
    void setSelection(const Akonadi::Tag::List &tags);
    /*!
     */
    [[nodiscard]] Akonadi::Tag::List selection() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::unique_ptr<TagEditWidgetPrivate> const d;
};

}
