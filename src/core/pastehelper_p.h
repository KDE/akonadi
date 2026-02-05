/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"

class KJob;
class QMimeData;

namespace Akonadi
{
class Session;

/*!
  \internal

  Helper methods for pasting/dropping content into a collection.

  \todo Use in item/collection models as well for dnd
*/
namespace PasteHelper
{
/*!
  Check whether the given URL list obtained from clipboard can be pasted into the given collection.
  \a clipboardUrls The pasted/dropped data (URL list).
  \a collection The collection to paste/drop into.
  \a action Indicate whether this is a copy, a move or link.
*/
AKONADICORE_EXPORT bool canPaste(const QList<QUrl> &clipboardUrls, const Collection &collection, Qt::DropAction action);

/*!
  Check whether the given mime data can be pasted into the given collection.
  \a mimeData The pasted/dropped data.
  \a collection The collection to paste/drop into.
  \a action Indicate whether this is a copy, a move or link.
*/
AKONADICORE_EXPORT bool canPaste(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action);

/*!
  Paste/drop the given mime data into the given collection.
  \a mimeData The pasted/dropped data.
  \a collection The target collection.
  \a action Indicate whether this is a copy, a move or link.
  Returns The job performing the paste, 0 if there is nothing to paste.
*/
AKONADICORE_EXPORT KJob *paste(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action, Session *session = nullptr);

/*!
  URI list paste/drop.
  \a mimeData The pasted/dropped data.
  \a collection The target collection.
  \a action The drop action (copy/move/link).
  Returns The job performing the paste, 0 if there is nothing to paste.
*/
AKONADICORE_EXPORT KJob *pasteUriList(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action, Session *session = nullptr);
}

}
