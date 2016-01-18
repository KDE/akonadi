/*
    Author: (2013) Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "gidextractor_p.h"
#include "gidextractorinterface.h"

#include "item.h"
#include "typepluginloader_p.h"

namespace Akonadi
{

QString GidExtractor::extractGid(const Item &item)
{
    const QObject *object = TypePluginLoader::objectForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    if (object) {
        const GidExtractorInterface *extractor = qobject_cast<GidExtractorInterface *>(object);
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

}
