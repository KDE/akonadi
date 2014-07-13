/*
    Copyright (c) 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef COLLECTIONTEST_H
#define COLLECTIONTEST_H

#include <QtCore/QObject>
#include "akonadi/collection.h"


class CollectionTest : public QObject {
  Q_OBJECT
  private:
    void verifyCollection( const Akonadi::Collection::List &colist, int listPosition,
                           const QString &remoteId, const QString &name,
                           const QStringList &mimeType );
  private Q_SLOTS:
    void serializeCollection();
    void testBuildCollection();
};

#endif
