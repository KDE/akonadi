/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "instance_p.h"

#include <QString>
#include <QGlobalStatic>

using namespace Akonadi;

namespace
{

Q_GLOBAL_STATIC(QString, sIdentifier) // NOLINT(readability-redundant-member-init)

void loadIdentifier()
{
    *sIdentifier = QString::fromUtf8(qgetenv("AKONADI_INSTANCE"));
    if (sIdentifier->isNull()) {
        // QString is null by default, which means it wasn't initialized
        // yet. Set it to empty when it is initialized
        *sIdentifier = QStringLiteral(""); // clazy:exclude=empty-qstringliteral
    }
}
} // namespace

bool Instance::hasIdentifier()
{
    if (::sIdentifier->isNull()) {
        ::loadIdentifier();
    }
    return !sIdentifier->isEmpty();
}

void Instance::setIdentifier(const QString &identifier)
{
    if (identifier.isNull()) {
        qunsetenv("AKONADI_INSTANCE");
        *::sIdentifier = QStringLiteral(""); // clazy:exclude=empty-qstringliteral
    } else {
        *::sIdentifier = identifier;
        qputenv("AKONADI_INSTANCE", identifier.toUtf8());
    }
}

QString Instance::identifier()
{
    if (::sIdentifier->isNull()) {
        ::loadIdentifier();
    }
    return *::sIdentifier;
}
