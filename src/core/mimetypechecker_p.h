/*
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef MIMETYPECHECKER_P_H
#define MIMETYPECHECKER_P_H

#include <QMimeType>
#include <QMimeDatabase>

#include <QtCore/QSet>
#include <QtCore/QStringList>

namespace Akonadi {

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

        foreach (const QString &wantedMimeType, mWantedMimeTypes) {
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

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
