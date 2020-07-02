/*
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

bool MimeTypeChecker::hasWantedMimeTypes() const
{
    return !d->mWantedMimeTypes.isEmpty();
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

    for (const QString &mimeType : contentMimeTypes) {
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

    for (const QString &mimeType : contentMimeTypes) {
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
    for (const QString &mt : mimeTypes) {
        if (d->isWantedMimeType(mt)) {
            return true;
        }
    }
    return false;
}

