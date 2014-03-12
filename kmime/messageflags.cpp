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

#include "messageflags.h"

const char *Akonadi::MessageFlags::Seen = "\\SEEN";
const char *Akonadi::MessageFlags::Deleted = "\\DELETED";
const char *Akonadi::MessageFlags::Answered = "\\ANSWERED";
const char *Akonadi::MessageFlags::Flagged = "\\FLAGGED";
const char *Akonadi::MessageFlags::HasError = "$ERROR";
const char *Akonadi::MessageFlags::HasAttachment = "$ATTACHMENT";
const char *Akonadi::MessageFlags::HasInvitation = "$INVITATION";
const char *Akonadi::MessageFlags::Sent = "$SENT";
const char *Akonadi::MessageFlags::Queued = "$QUEUED";
const char *Akonadi::MessageFlags::Replied = "$REPLIED";
const char *Akonadi::MessageFlags::Forwarded = "$FORWARDED";
const char *Akonadi::MessageFlags::ToAct = "$TODO";
const char *Akonadi::MessageFlags::Watched = "$WATCHED";
const char *Akonadi::MessageFlags::Ignored = "$IGNORED";
const char *Akonadi::MessageFlags::Signed = "$SIGNED";
const char *Akonadi::MessageFlags::Encrypted = "$ENCRYPTED";
const char *Akonadi::MessageFlags::Spam = "$JUNK";
const char *Akonadi::MessageFlags::Ham = "$NOTJUNK";
