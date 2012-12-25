/*
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#include <Akonadi/Calendar/ETMCalendar>
#include <KCheckableProxyModel>

#include <QApplication>
#include <QTreeView>
#include <QHBoxLayout>

using namespace Akonadi;

int main( int argv, char **argc )
{
  QApplication app( argv, argc );

  ETMCalendar calendar;

  QWidget *window = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout( window );

  QTreeView *collectionSelectionView = new QTreeView();
  collectionSelectionView->setModel( calendar.checkableProxyModel() );

  QTreeView *itemView = new QTreeView();
  itemView->setModel( calendar.model() );

  layout->addWidget( collectionSelectionView );
  layout->addWidget( itemView );

  window->show();

  return app.exec();
}
