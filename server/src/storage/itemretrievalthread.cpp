/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "itemretrievalthread.h"
#include "itemretrievalmanager.h"

#include <QCoreApplication>

using namespace Akonadi::Server;

ItemRetrievalThread::ItemRetrievalThread( QObject *parent )
  : QThread( parent )
{
  // make sure we are created from the main thread, ie. before all other threads start to potentially use us
  Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );
  setObjectName( QLatin1String( "ItemRetrievalManager-Thread" ) );
}

void ItemRetrievalThread::run()
{
  ItemRetrievalManager *mgr = new ItemRetrievalManager();
  exec();
  delete mgr;
}
