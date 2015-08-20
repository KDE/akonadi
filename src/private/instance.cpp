/*
 * Copyright 2015  Daniel Vratil <dvratil@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "instance_p.h"

#include <QtCore/QString>

using namespace Akonadi;

QString Instance::sIdentifier = QString();

void Instance::loadIdentifier()
{
    sIdentifier = QString::fromUtf8(qgetenv("AKONADI_INSTANCE"));
    if (sIdentifier.isNull()) {
        // QString is null by default, which means it wasn't initialized
        // yet. Set it to empty when it is initialized
        sIdentifier = QStringLiteral("");
    }
}

bool Instance::hasIdentifier()
{
    if (sIdentifier.isNull()) {
        loadIdentifier();
    }
    return !sIdentifier.isEmpty();
}

void Instance::setIdentifier(const QString &identifier)
{
    if (identifier.isNull()) {
        qunsetenv("AKONADI_INSTANCE");
        sIdentifier = QStringLiteral("");
    } else {
        sIdentifier = identifier;
        qputenv("AKONADI_INSTANCE", identifier.toUtf8());
    }
}

QString Instance::identifier()
{
    if (sIdentifier.isNull()) {
        loadIdentifier();
    }
    return sIdentifier;
}
