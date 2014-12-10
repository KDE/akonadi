/*
 *  Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
 *  Copyright (C) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "entityannotationsattribute.h"

#include <QByteArray>
#include <QString>

using namespace Akonadi;

EntityAnnotationsAttribute::EntityAnnotationsAttribute()
{
}

EntityAnnotationsAttribute::EntityAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations)
    : mAnnotations(annotations)
{
}

void EntityAnnotationsAttribute::setAnnotations(const QMap<QByteArray, QByteArray> &annotations)
{
    mAnnotations = annotations;
}

QMap<QByteArray, QByteArray> EntityAnnotationsAttribute::annotations() const
{
    return mAnnotations;
}

void EntityAnnotationsAttribute::insert(const QByteArray &key, const QString &value)
{
    mAnnotations.insert(key, value.toUtf8());
}

QString EntityAnnotationsAttribute::value(const QByteArray &key)
{
    return QString::fromUtf8(mAnnotations.value(key).data());
}

bool EntityAnnotationsAttribute::contains(const QByteArray &key) const
{
    return mAnnotations.contains(key);
}

QByteArray EntityAnnotationsAttribute::type() const
{
    static const QByteArray sType( "entityannotations" );
    return sType;
}

Akonadi::Attribute *EntityAnnotationsAttribute::clone() const
{
    return new EntityAnnotationsAttribute(mAnnotations);
}

QByteArray EntityAnnotationsAttribute::serialized() const
{
    QByteArray result = "";

    Q_FOREACH (const QByteArray &key, mAnnotations.keys()) {
        result += key;
        result += ' ';
        result += mAnnotations[key];
        result += " % "; // We use this separator as '%' is not allowed in keys or values
    }
    result.chop(3);

    return result;
}

void EntityAnnotationsAttribute::deserialize(const QByteArray &data)
{
    mAnnotations.clear();
    const QList<QByteArray> lines = data.split('%');

    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i];
        if (i != 0 && line.startsWith(' ')) {
            line = line.mid(1);
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
