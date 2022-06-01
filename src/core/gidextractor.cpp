/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gidextractor_p.h"
#include "gidextractorinterface.h"

#include "item.h"
#include "typepluginloader_p.h"

using namespace Akonadi;

QString GidExtractor::extractGid(const Item &item)
{
    const QObject *object = TypePluginLoader::objectForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    if (object) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const GidExtractorInterface *extractor = qobject_cast<const GidExtractorInterface *>(object);
#else
        const GidExtractorInterface *extractor = qobject_cast<GidExtractorInterface *>(object);
#endif
        if (extractor) {
            return extractor->extractGid(item);
        }
    }
    return QString();
}

QString GidExtractor::getGid(const Item &item)
{
    const QString gid = item.gid();
    if (!gid.isNull()) {
        return gid;
    }
    if (item.loadedPayloadParts().isEmpty()) {
        return QString();
    }
    return extractGid(item);
}
