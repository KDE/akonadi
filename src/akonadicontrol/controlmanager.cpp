/***************************************************************************
 *   SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "controlmanager.h"

#include <QCoreApplication>
#include <QTimer>

#include "controlmanageradaptor.h"

ControlManager::ControlManager(QObject *parent)
    : QObject(parent)
{
    new ControlManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ControlManager"), this);
}

ControlManager::~ControlManager()
{
}

void ControlManager::shutdown()
{
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
}
