/*
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMimeDatabase>
#include <QMimeType>

#include <QSet>

namespace Akonadi
{
/**
 * @internal
 */
class MimeTypeCheckerPrivate : public QSharedData
{
public:
    MimeTypeCheckerPrivate()
    {
    }

    MimeTypeCheckerPrivate(const MimeTypeCheckerPrivate &other)
        : QSharedData(other)
    {
        mWantedMimeTypes = other.mWantedMimeTypes;
    }

    bool isWantedMimeType(const QString &mimeType) const
    {
        if (mWantedMimeTypes.contains(mimeType)) {
            return true;
        }

        QMimeDatabase db;
        const QMimeType mt = db.mimeTypeForName(mimeType);
        if (!mt.isValid()) {
            return false;
        }

        for (const QString &wantedMimeType : std::as_const(mWantedMimeTypes)) {
            if (mt.inherits(wantedMimeType)) {
                return true;
            }
        }

        return false;
    }

public:
    QSet<QString> mWantedMimeTypes;
};

}
