/*
 *  SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>
 *  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <QMap>

namespace Akonadi
{
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
    explicit EntityAnnotationsAttribute() = default;
    explicit EntityAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations);

    void setAnnotations(const QMap<QByteArray, QByteArray> &annotations);
    Q_REQUIRED_RESULT QMap<QByteArray, QByteArray> annotations() const;

    void insert(const QByteArray &key, const QString &value);
    Q_REQUIRED_RESULT QString value(const QByteArray &key) const;
    Q_REQUIRED_RESULT bool contains(const QByteArray &key) const;

    Q_REQUIRED_RESULT QByteArray type() const override;
    Attribute *clone() const override;
    Q_REQUIRED_RESULT QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QMap<QByteArray, QByteArray> mAnnotations;
};

} // namespace Akonadi
