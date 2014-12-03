/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Tobias Koenig <tokoe@kdab.com>

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

#include "blockalarmsattribute.h"
#include <akonadi/attributefactory.h>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>

using namespace Akonadi;

class BlockAlarmsAttribute::Private
{
public:
    Private():
        audio(1),
        display(1),
        email(1),
        procedure(1)
    { }

    int audio : 1;
    int display : 1;
    int email : 1;
    int procedure : 1;
};

BlockAlarmsAttribute::BlockAlarmsAttribute():
    d(new Private)
{
}

BlockAlarmsAttribute::~BlockAlarmsAttribute()
{
    delete d;
}

void BlockAlarmsAttribute::blockAlarmType(KCalCore::Alarm::Type type, bool block)
{
    switch (type) {
    case KCalCore::Alarm::Audio:
        d->audio = block;
        return;
    case KCalCore::Alarm::Display:
        d->display = block;
        return;
    case KCalCore::Alarm::Email:
        d->email = block;
        return;
    case KCalCore::Alarm::Procedure:
        d->procedure = block;
        return;
    default:
        return;
    }
}

void BlockAlarmsAttribute::blockEverything(bool block)
{
    blockAlarmType(KCalCore::Alarm::Audio, block);
    blockAlarmType(KCalCore::Alarm::Display, block);
    blockAlarmType(KCalCore::Alarm::Email, block);
    blockAlarmType(KCalCore::Alarm::Procedure, block);
}

bool BlockAlarmsAttribute::isAlarmTypeBlocked(KCalCore::Alarm::Type type) const
{
    switch (type) {
    case KCalCore::Alarm::Audio:
        return d->audio;
    case KCalCore::Alarm::Display:
        return d->display;
    case KCalCore::Alarm::Email:
        return d->email;
    case KCalCore::Alarm::Procedure:
        return d->procedure;
    default:
        return false;
    }
}

bool BlockAlarmsAttribute::isEverythingBlocked() const
{
    return isAlarmTypeBlocked(KCalCore::Alarm::Audio) && isAlarmTypeBlocked(KCalCore::Alarm::Display)
        && isAlarmTypeBlocked(KCalCore::Alarm::Email) && isAlarmTypeBlocked(KCalCore::Alarm::Procedure);
}

QByteArray BlockAlarmsAttribute::type() const
{
    return "BlockAlarmsAttribute";
}

BlockAlarmsAttribute *BlockAlarmsAttribute::clone() const
{
    BlockAlarmsAttribute *copy = new BlockAlarmsAttribute();
    copy->d->audio = d->audio;
    copy->d->display = d->display;
    copy->d->email = d->email;
    copy->d->procedure = d->procedure;

    return copy;
}

QByteArray BlockAlarmsAttribute::serialized() const
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << d->audio;
    stream << d->display;
    stream << d->email;
    stream << d->procedure;

    return ba;
}

void BlockAlarmsAttribute::deserialize(const QByteArray &data)
{
    // Pre-4.11, default behavior
    if (data.isEmpty()) {
        d->audio = true;
        d->display = true;
        d->email = true;
        d->procedure = true;
    } else {
        QByteArray ba = data;
        QDataStream stream(&ba, QIODevice::ReadOnly);
        int i;
        stream >> i;
        d->audio = i;
        stream >> i;
        d->display = i;
        stream >> i;
        d->email = i;
        stream >> i;
        d->procedure = i;
    }
}

#ifndef KDELIBS_STATIC_LIBS
namespace {

// Anonymous namespace; function is invisible outside this file.
bool dummy()
{
    Akonadi::AttributeFactory::registerAttribute<Akonadi::BlockAlarmsAttribute>();

    return true;
}

// Called when this library is loaded.
const bool registered = dummy();

} // namespace

#else

extern bool ___Akonadi____INIT()
{
    Akonadi::AttributeFactory::registerAttribute<Akonadi::BlockAlarmsAttribute>();

    return true;
}

#endif
