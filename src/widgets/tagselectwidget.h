/*
  SPDX-FileCopyrightText: 2015-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
  */

#pragma once

#include "akonadiwidgets_export.h"
// AkonadiCore
#include <akonadi/tag.h>

#include <QWidget>

#include <memory>

namespace Akonadi
{
class TagSelectWidgetPrivate;

/**
 * A widget that offers facilities to add/remove tags and provides a way to select tags.
 *
 * @since 4.14.6
 */

class AKONADIWIDGETS_EXPORT TagSelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TagSelectWidget(QWidget *parent = nullptr);
    ~TagSelectWidget() override;

    void setSelection(const Akonadi::Tag::List &tags);
    [[nodiscard]] Akonadi::Tag::List selection() const;

    /**
     * @brief tagToStringList
     * @return QStringList from selected tag (List of Url)
     */
    [[nodiscard]] QStringList tagToStringList() const;
    /**
     * @brief setSelectionFromStringList, convert a QStringList to Tag (converted from url)
     */
    void setSelectionFromStringList(const QStringList &lst);

private:
    /// @cond PRIVATE
    std::unique_ptr<TagSelectWidgetPrivate> const d;
    /// @endcond
};
}
