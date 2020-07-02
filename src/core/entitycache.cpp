/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitycache_p.h"

using namespace Akonadi;

EntityCacheBase::EntityCacheBase(Session *_session, QObject *parent)
    : QObject(parent)
    , session(_session)
{
}

void EntityCacheBase::setSession(Session *_session)
{
    session = _session;
}

#include "moc_entitycache_p.cpp"
