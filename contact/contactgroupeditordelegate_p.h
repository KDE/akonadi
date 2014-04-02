/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTGROUPEDITORDELEGATE_P_H
#define AKONADI_CONTACTGROUPEDITORDELEGATE_P_H

#include <QLineEdit>

#include <QStyledItemDelegate>

#include <akonadi/item.h>

namespace Akonadi
{

class ContactLineEdit : public QLineEdit
{
  Q_OBJECT

  public:
    explicit ContactLineEdit( bool isReference, QWidget *parent = 0 );

    bool isReference() const;
    Akonadi::Item completedItem() const;

  Q_SIGNALS:
    void completed( QWidget* );

  private Q_SLOTS:
    void completed( const QModelIndex& );
    void slotTextEdited();

  private:
    bool mIsReference;
    Item mItem;
};

class ContactGroupEditorDelegate : public QStyledItemDelegate
{
  Q_OBJECT

  public:
    explicit ContactGroupEditorDelegate( QAbstractItemView *view, QObject *parent = 0 );
    ~ContactGroupEditorDelegate();

    virtual QWidget* createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

    virtual void setEditorData( QWidget *editor, const QModelIndex &index ) const;
    virtual void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const;

    virtual void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual bool editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index );

  private Q_SLOTS:
    void completed( QWidget* );
    void setFirstColumnAsCurrent();

  private:
    class Private;
    Private* const d;
};

}

#endif
