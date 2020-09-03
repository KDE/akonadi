/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cookiemanager.h"

#include <array>

#include <QRandomGenerator>

using namespace Akonadi::Server;

Cookie CookieManager::generateCookie()
{
    std::array<uint32_t, 10> data;
    QRandomGenerator::system()->generate(data.begin(), data.end());

    return Cookie{reinterpret_cast<const char *>(data.data()), data.size()}.toHex();
}

CookieData &CookieManager::data(const Cookie &cookie)
{
    std::scoped_lock lock(mMutex);
    auto data = mCookies.find(cookie);
    if (data == mCookies.end()) {
        data = mCookies.insert(cookie, CookieData{});
    }

    return *data;
}

void CookieManager::setData(const Cookie &cookie, const CookieData &data)
{
    std::scoped_lock lock(mMutex);
    mCookies[cookie] = data;
}
