/*
  This file is part of Akonadi Contact.

  Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "imdelegate.h"

#include "immodel.h"

IMDelegate::IMDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

IMDelegate::~IMDelegate()
{
}

QWidget *IMDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &item,
                                  const QModelIndex &index) const
{
    Q_UNUSED(parent);
    Q_UNUSED(item);
    Q_UNUSED(index);
    return 0;
}

void IMDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const
{
    QStyleOptionViewItem newOption(option);
    if (index.data(IMModel::IsPreferredRole).toBool()) {
        newOption.font.setBold(true);
    }

    QStyledItemDelegate::paint(painter, newOption, index);
}
