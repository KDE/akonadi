/*
 *  SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>
 *  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "entityannotationsattribute.h"

#include <QByteArray>
#include <QString>

using namespace Akonadi;

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

QString EntityAnnotationsAttribute::value(const QByteArray &key) const
{
    const auto val = mAnnotations.value(key);
    return QString::fromUtf8(val.data());
}

bool EntityAnnotationsAttribute::contains(const QByteArray &key) const
{
    return mAnnotations.contains(key);
}

QByteArray EntityAnnotationsAttribute::type() const
{
    static const QByteArray sType("entityannotations");
    return sType;
}

Akonadi::Attribute *EntityAnnotationsAttribute::clone() const
{
    return new EntityAnnotationsAttribute(mAnnotations);
}

QByteArray EntityAnnotationsAttribute::serialized() const
{
    QByteArray result = "";

    for (auto it = mAnnotations.cbegin(), e = mAnnotations.cend(); it != e; ++it) {
        result += it.key();
        result += ' ';
        result += it.value();
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
        const int wsIndex = line.indexOf(' ');
        if (wsIndex > 0) {
            const QByteArray key = line.mid(0, wsIndex);
            const QByteArray value = line.mid(wsIndex + 1);
            mAnnotations[key] = value;
        } else {
            mAnnotations.insert(line, QByteArray());
        }
    }
}
