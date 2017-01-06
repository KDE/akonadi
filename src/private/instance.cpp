/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "instance_p.h"

#include <QString>

using namespace Akonadi;

namespace
{
static QString sIdentifier;

static void loadIdentifier()
{
    sIdentifier = QString::fromUtf8(qgetenv("AKONADI_INSTANCE"));
    if (sIdentifier.isNull()) {
        // QString is null by default, which means it wasn't initialized
        // yet. Set it to empty when it is initialized
        sIdentifier = QStringLiteral("");
    }
}
}

bool Instance::hasIdentifier()
{
    if (::sIdentifier.isNull()) {
        ::loadIdentifier();
    }
    return !sIdentifier.isEmpty();
}

void Instance::setIdentifier(const QString &identifier)
{
    if (identifier.isNull()) {
        qunsetenv("AKONADI_INSTANCE");
        ::sIdentifier = QStringLiteral("");
    } else {
        ::sIdentifier = identifier;
        qputenv("AKONADI_INSTANCE", identifier.toUtf8());
    }
}

QString Instance::identifier()
{
    if (::sIdentifier.isNull()) {
        ::loadIdentifier();
    }
    return ::sIdentifier;
}
