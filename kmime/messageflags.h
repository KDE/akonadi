/*
 * Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef AKONADI_MESSAGEFLAGS_H
#define AKONADI_MESSAGEFLAGS_H

#include "akonadi-kmime_export.h"

namespace Akonadi
{
  /**
   * @short Contains predefined message flag identifiers.
   *
   * This namespace contains identifiers of message flags that
   *  are used internally in the Akonadi server.
   */
  namespace MessageFlags
  {
    /**
     * The flag for a message being seen (i.e. opened by user).
     */
    AKONADI_KMIME_EXPORT extern const char* Seen;

    /**
     * The flag for a message being deleted by the user.
     */
    AKONADI_KMIME_EXPORT extern const char* Deleted;

    /**
     * The flag for a message being replied to by the user.
     */
    AKONADI_KMIME_EXPORT extern const char* Answered;

    /**
     * The flag for a message being marked as flagged.
     */
    AKONADI_KMIME_EXPORT extern const char* Flagged;

    /**
     * The flag for a message being marked as having an attachment.
     */
    AKONADI_KMIME_EXPORT extern const char* Attachment;

  }
}

#endif
