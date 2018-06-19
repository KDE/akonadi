/*
  Copyright (c) 2015-2018 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
  */

#ifndef TAGSELECTWIDGET_H
#define TAGSELECTWIDGET_H

#include <QWidget>
#include "tag.h"
#include "akonadiwidgets_export.h"

namespace Akonadi
{
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
    ~TagSelectWidget();

    void setSelection(const Akonadi::Tag::List &tags);
    Q_REQUIRED_RESULT Akonadi::Tag::List selection() const;

    /**
     * @brief tagToStringList
     * @return QStringList from selected tag (List of Url)
     */
    Q_REQUIRED_RESULT QStringList tagToStringList() const;
    /**
     * @brief setSelectionFromStringList, convert a QStringList to Tag (converted from url)
     */
    void setSelectionFromStringList(const QStringList &lst);
private:
    //@cond PRIVATE
    class Private;
    Private *const d;
};
}

#endif // TAGSELECTWIDGET_H
