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
#include "improtocols.h"

#include <kcombobox.h>
#include <kicon.h>
#include <klocale.h>

IMDelegate::IMDelegate( QObject *parent )
  : QStyledItemDelegate( parent )
{
}

IMDelegate::~IMDelegate()
{
}

QWidget* IMDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &item, const QModelIndex &index ) const
{
  if ( index.column() == 0 ) {
    KComboBox *comboBox = new KComboBox( parent );
    comboBox->setFrame( false );
    comboBox->setAutoFillBackground( true );

    const QStringList protocols = IMProtocols::self()->protocols();
    foreach ( const QString &protocol, protocols ) {
      comboBox->addItem( KIcon( IMProtocols::self()->icon( protocol ) ),
                         IMProtocols::self()->name( protocol ),
                         protocol );
    }

    return comboBox;
  } else {
    return QStyledItemDelegate::createEditor( parent, item, index );
  }
}

void IMDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  if ( index.column() == 0 ) {
    KComboBox *comboBox = qobject_cast<KComboBox*>( editor );
    if ( !comboBox )
      return;

    const QString protocol = index.data( IMModel::ProtocolRole ).toString();
    comboBox->setCurrentIndex( comboBox->findData( protocol ) );
  } else {
    QStyledItemDelegate::setEditorData( editor, index );
  }
}

void IMDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  if ( index.column() == 0 ) {
    KComboBox *comboBox = qobject_cast<KComboBox*>( editor );
    if ( !comboBox )
      return;

    model->setData( index, comboBox->itemData( comboBox->currentIndex() ), IMModel::ProtocolRole );
  } else {
    QStyledItemDelegate::setModelData( editor, model, index );
  }
}

void IMDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( index.data( IMModel::IsPreferredRole ).toBool() ) {
    QStyleOptionViewItem newOption( option );
    newOption.font.setBold( true );

    QStyledItemDelegate::paint( painter, newOption, index );
  } else
    QStyledItemDelegate::paint( painter, option, index );
}

