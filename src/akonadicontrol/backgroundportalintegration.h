/*
 *   SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QVariantMap>

class QDBusPendingCallWatcher;

class BackgroundPortalIntegration : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundPortalIntegration(QObject *parent = nullptr);

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *watcher);
    void onBackgroundResponse(uint responseCode);

private:
    void requestBackgroundPermission();
};
