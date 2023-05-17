/*
  SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionannotationsattribute.h"

#include <QByteArray>

using namespace Akonadi;

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute() = default;

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations)
    : mAnnotations(annotations)
{
}

void CollectionAnnotationsAttribute::setAnnotations(const QMap<QByteArray, QByteArray> &annotations)
{
    mAnnotations = annotations;
}

QMap<QByteArray, QByteArray> CollectionAnnotationsAttribute::annotations() const
{
    return mAnnotations;
}

QByteArray CollectionAnnotationsAttribute::type() const
{
    static const QByteArray sType("collectionannotations");
    return sType;
}

CollectionAnnotationsAttribute *CollectionAnnotationsAttribute::clone() const
{
    return new CollectionAnnotationsAttribute(mAnnotations);
}

QByteArray CollectionAnnotationsAttribute::serialized() const
{
    QByteArray result;
    for (auto it = mAnnotations.cbegin(), end = mAnnotations.cend(); it != end; ++it) {
        result += it.key();
        result += ' ';
        result += it.value();
        result += " % "; // We use this separator as '%' is not allowed in keys or values
    }
    result.chop(3);

    return result;
}

void CollectionAnnotationsAttribute::deserialize(const QByteArray &data)
{
    mAnnotations.clear();
    const QList<QByteArray> lines = data.split('%');

    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i];
        if (i != 0 && line.startsWith(' ')) {
            line.remove(0, 1);
        }
        if (i != lines.size() - 1 && line.endsWith(' ')) {
            line.chop(1);
        }
        if (line.trimmed().isEmpty()) {
            continue;
        }
        int wsIndex = line.indexOf(' ');
        if (wsIndex > 0) {
            const QByteArray key = line.mid(0, wsIndex);
            const QByteArray value = line.mid(wsIndex + 1);
            mAnnotations[key] = value;
        } else {
            mAnnotations.insert(line, QByteArray());
        }
    }
}

bool CollectionAnnotationsAttribute::operator==(const CollectionAnnotationsAttribute &other) const
{
    return mAnnotations == other.annotations();
}
