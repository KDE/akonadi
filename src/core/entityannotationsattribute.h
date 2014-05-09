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

#ifndef AKONADI_ENTITYANNOTATIONSATTRIBUTE_H
#define AKONADI_ENTITYANNOTATIONSATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <QMap>

namespace Akonadi {

/**
 * An attribute for annotations.
 *
 * The attribute is inspired by RFC5257(IMAP ANNOTATION) and RFC5464(IMAP METADATA), but serves
 * the purpose of RFC5257.
 *
 * For a private note annotation the entry name is:
 * /private/comment
 * for a shared note:
 * /shared/comment
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT EntityAnnotationsAttribute : public Akonadi::Attribute
{
public:
    EntityAnnotationsAttribute();
    EntityAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations);

    void setAnnotations(const QMap<QByteArray, QByteArray> &annotations);
    QMap<QByteArray, QByteArray> annotations() const;

    void insert(const QByteArray &key, const QString &value);
    QString value(const QByteArray &key);
    bool contains(const QByteArray &key) const;

    QByteArray type() const;
    Attribute *clone() const;
    QByteArray serialized() const;
    void deserialize(const QByteArray &data);

private:
    QMap<QByteArray, QByteArray> mAnnotations;
};

}

#endif
