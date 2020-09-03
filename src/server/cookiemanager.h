/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SERVER_COOKIEMANAGER_H
#define AKONADI_SERVER_COOKIEMANAGER_H

#include <chrono>
#include <optional>
#include <mutex>

#include <QHash>
#include <QString>
#include <QByteArray>

namespace Akonadi::Server
{

using Cookie = QByteArray;

struct CookieData
{
    std::chrono::time_point<std::chrono::steady_clock> created;
    std::chrono::seconds validFor;

    std::optional<QString> resourceId;
};


class CookieManager
{
public:
    Cookie generateCookie();

    CookieData &data(const Cookie &cookie);
    void setData(const Cookie &cookie, const CookieData &data);

private:
    std::mutex mMutex;
    QHash<Cookie, CookieData> mCookies;
};

} // namespace Akonadi::Server

#endif


