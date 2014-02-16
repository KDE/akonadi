/*
  Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

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

// No point on having warnings. This class won't be ported to KCalCore
// and will be deleted in KDE5.
#define WANT_DEPRECATED_KCAL_API

#include "incidencemimetypevisitor.h"

static QLatin1String sEventType("application/x-vnd.akonadi.calendar.event");
static QLatin1String sTodoType("application/x-vnd.akonadi.calendar.todo");
static QLatin1String sJournalType("application/x-vnd.akonadi.calendar.journal");
static QLatin1String sFreeBusyType("application/x-vnd.akonadi.calendar.freebusy");

using namespace Akonadi;

class IncidenceMimeTypeVisitor::Private
{
public:
    QString mType;
};

IncidenceMimeTypeVisitor::IncidenceMimeTypeVisitor()
    : d(new Private())
{
}

IncidenceMimeTypeVisitor::~IncidenceMimeTypeVisitor()
{
    delete d;
}

bool IncidenceMimeTypeVisitor::visit(KCal::Event *event)
{
    Q_UNUSED(event);
    d->mType = sEventType;
    return true;
}

bool IncidenceMimeTypeVisitor::visit(KCal::Todo *todo)
{
    Q_UNUSED(todo);
    d->mType = sTodoType;
    return true;
}

bool IncidenceMimeTypeVisitor::visit(KCal::Journal *journal)
{
    Q_UNUSED(journal);
    d->mType = sJournalType;
    return true;
}

bool IncidenceMimeTypeVisitor::visit(KCal::FreeBusy *freebusy)
{
    Q_UNUSED(freebusy);
    d->mType = sFreeBusyType;
    return true;
}

QString IncidenceMimeTypeVisitor::mimeType() const
{
    return d->mType;
}

QStringList IncidenceMimeTypeVisitor::allMimeTypes() const
{
    return QStringList() << sEventType << sTodoType << sJournalType << sFreeBusyType;
}

QString IncidenceMimeTypeVisitor::mimeType(KCal::IncidenceBase *incidence)
{
    Q_ASSERT(incidence != 0);

    incidence->accept(*this);
    return mimeType();
}

QString IncidenceMimeTypeVisitor::eventMimeType()
{
    return sEventType;
}

QString IncidenceMimeTypeVisitor::todoMimeType()
{
    return sTodoType;
}

QString IncidenceMimeTypeVisitor::journalMimeType()
{
    return sJournalType;
}

QString IncidenceMimeTypeVisitor::freeBusyMimeType()
{
    return sFreeBusyType;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
