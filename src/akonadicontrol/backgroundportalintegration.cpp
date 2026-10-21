/*
 *   SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "backgroundportalintegration.h"
#include "akonadicontrol_debug.h"

#include <QDBusConnection>
#include <QDBusPendingReply>

using namespace Qt::Literals;

BackgroundPortalIntegration::BackgroundPortalIntegration(QObject *parent)
    : QObject(parent)
{
    requestBackgroundPermission();
}

void BackgroundPortalIntegration::requestBackgroundPermission()
{
    QDBusMessage portalMessage = QDBusMessage::createMethodCall(u"org.freedesktop.portal.Desktop"_s,
                                                                u"/org/freedesktop/portal/desktop"_s,
                                                                u"org.freedesktop.portal.Background"_s,
                                                                u"RequestBackground"_s);

    portalMessage.setArguments({u""_s, QVariantMap{{u"autostart"_s, true}}});

    QDBusPendingCall asyncCall = QDBusConnection::sessionBus().asyncCall(portalMessage);
    auto *watcher = new QDBusPendingCallWatcher(asyncCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BackgroundPortalIntegration::onCallFinished);
}

void BackgroundPortalIntegration::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    if (reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Background portal request failed:" << reply.error().message();
        return;
    }

    const QDBusObjectPath requestPath = reply.value();

    QDBusConnection::sessionBus().connect(u"org.freedesktop.portal.Desktop"_s,
                                          requestPath.path(),
                                          u"org.freedesktop.portal.Request"_s,
                                          u"Response"_s,
                                          this,
                                          SLOT(onBackgroundResponse(uint)));
}

void BackgroundPortalIntegration::onBackgroundResponse(uint responseCode)
{
    if (responseCode != 0) {
        qCWarning(AKONADICONTROL_LOG) << "Background portal request dismissed or denied with code:" << responseCode;
    }
}

#include "moc_backgroundportalintegration.cpp"
