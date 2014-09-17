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

#include "mimetypechecker.h"

#include "mimetypechecker_p.h"

#include "collection.h"
#include "item.h"

using namespace Akonadi;

MimeTypeChecker::MimeTypeChecker()
{
    d = new MimeTypeCheckerPrivate();
}

MimeTypeChecker::MimeTypeChecker(const MimeTypeChecker &other)
    : d(other.d)
{
}

MimeTypeChecker::~MimeTypeChecker()
{
}

MimeTypeChecker &MimeTypeChecker::operator=(const MimeTypeChecker &other)
{
    if (&other != this) {
        d = other.d;
    }

    return *this;
}

QStringList MimeTypeChecker::wantedMimeTypes() const
{
    return d->mWantedMimeTypes.values();
}

void MimeTypeChecker::setWantedMimeTypes(const QStringList &mimeTypes)
{
    d->mWantedMimeTypes = QSet<QString>::fromList(mimeTypes);
}

void MimeTypeChecker::addWantedMimeType(const QString &mimeType)
{
    d->mWantedMimeTypes.insert(mimeType);
}

void MimeTypeChecker::removeWantedMimeType(const QString &mimeType)
{
    d->mWantedMimeTypes.remove(mimeType);
}

bool MimeTypeChecker::isWantedItem(const Item &item) const
{
    if (d->mWantedMimeTypes.isEmpty() || !item.isValid()) {
        return false;
    }

    const QString mimeType = item.mimeType();
    if (mimeType.isEmpty()) {
        return false;
    }

    return d->isWantedMimeType(mimeType);
}

bool MimeTypeChecker::isWantedCollection(const Collection &collection) const
{
    if (d->mWantedMimeTypes.isEmpty() || !collection.isValid()) {
        return false;
    }

    const QStringList contentMimeTypes = collection.contentMimeTypes();
    if (contentMimeTypes.isEmpty()) {
        return false;
    }

    foreach (const QString &mimeType, contentMimeTypes) {
        if (mimeType.isEmpty()) {
            continue;
        }

        if (d->isWantedMimeType(mimeType)) {
            return true;
        }
    }

    return false;
}

bool MimeTypeChecker::isWantedItem(const Item &item, const QString &wantedMimeType)
{
    if (wantedMimeType.isEmpty() || !item.isValid()) {
        return false;
    }

    const QString mimeType = item.mimeType();
    if (mimeType.isEmpty()) {
        return false;
    }

    if (mimeType == wantedMimeType) {
        return true;
    }

    QMimeDatabase db;
    const QMimeType mt = db.mimeTypeForName(mimeType);
    if (!mt.isValid()) {
        return false;
    }

    return mt.inherits(wantedMimeType);
}

bool MimeTypeChecker::isWantedCollection(const Collection &collection, const QString &wantedMimeType)
{
    if (wantedMimeType.isEmpty() || !collection.isValid()) {
        return false;
    }

    const QStringList contentMimeTypes = collection.contentMimeTypes();
    if (contentMimeTypes.isEmpty()) {
        return false;
    }

    foreach (const QString &mimeType, contentMimeTypes) {
        if (mimeType.isEmpty()) {
            continue;
        }

        if (mimeType == wantedMimeType) {
            return true;
        }

        QMimeDatabase db;
        const QMimeType mt = db.mimeTypeForName(mimeType);
        if (!mt.isValid()) {
            continue;
        }

        if (mt.inherits(wantedMimeType)) {
            return true;
        }
    }

    return false;
}

bool MimeTypeChecker::isWantedMimeType(const QString &mimeType) const
{
    return d->isWantedMimeType(mimeType);
}

bool MimeTypeChecker::containsWantedMimeType(const QStringList &mimeTypes) const
{
    foreach (const QString &mt, mimeTypes) {
        if (d->isWantedMimeType(mt)) {
            return true;
        }
    }
    return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
