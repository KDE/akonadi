// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "mimetypes.h"

using namespace Akonadi::Quick;
using namespace Qt::StringLiterals;

MimeTypes::MimeTypes(QObject *parent)
    : QObject(parent)
{
}

QString MimeTypes::calendar() const
{
    return u"application/x-vnd.akonadi.calendar.event"_s;
}

QString MimeTypes::todo() const
{
    return u"application/x-vnd.akonadi.calendar.todo"_s;
}

QString MimeTypes::address() const
{
    return u"text/directory"_s;
}

QString MimeTypes::contactGroup() const
{
    return u"application/x-vnd.kde.contactgroup"_s;
}

QString MimeTypes::mail() const
{
    return u"message/rfc822"_s;
}

QString MimeTypes::journal() const
{
    return u"application/x-vnd.akonadi.calendar.journal"_s;
}

#include "moc_mimetypes.cpp"
