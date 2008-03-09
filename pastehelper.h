/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_PASTEHELPER_H
#define AKONADI_PASTEHELPER_H

#include <libakonadi/collection.h>

#include <QtCore/QList>

class KJob;
class QMimeData;

namespace Akonadi {

/**
  @internal

  Helper methods for pasting/droping content into a collection.

  @todo Use in item/collection models as well for dnd
*/
namespace PasteHelper
{
  /**
    Check wether the given mime data can be pasted into the given collection.
    @param mimeData The pasted/dropped data.
    @param collection The collection to paste/drop into.
  */
  bool canPaste( const QMimeData* mimeData, const Collection &collection );

  /**
    Paste/drop the given mime data into the given collection.
    @param mimeData The pasted/dropped data.
    @param collection The target collection.
    @param copy Indicate wether this is a copy or a move.
    @returns The job performing the paste, 0 if there is nothing to paste.
  */
  KJob* paste( const QMimeData* mimeData, const Collection &collection, bool copy = true );
}


}

#endif
