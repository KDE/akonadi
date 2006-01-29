/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collection.h"
#include "collectionlistjob.h"

#include <QHash>
#include <QTimer>

using namespace PIM;

PIM::CollectionListJob::CollectionListJob()
{
}

PIM::CollectionListJob::~CollectionListJob()
{
}

QHash<DataReference, Collection*> PIM::CollectionListJob::collections() const
{
  return QHash<DataReference, Collection*>();
}

void PIM::CollectionListJob::doStart()
{
  QTimer::singleShot( 0, this, SLOT( emitDone() ) );
}

void PIM::CollectionListJob::emitDone()
{
  emit done( this );
}

#include "collectionlistjob.moc"
