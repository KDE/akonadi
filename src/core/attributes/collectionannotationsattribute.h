/*
  SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "attribute.h"

#include <QMap>

namespace Akonadi
{

/*!
 * Collection annotations attribute.
 *
 * This is primarily meant for storing IMAP ANNOTATION (RFC5257) data for resources
 * supporting that.
 *
 * \class Akonadi::CollectionAnnotationsAttribute
 * \inheaderfile Akonadi/CollectionAnnotationsAttribute
 * \inmodule AkonadiCore
 *
 * \since 5.23.43
 */
class AKONADICORE_EXPORT CollectionAnnotationsAttribute : public Akonadi::Attribute
{
public:
    /*!
     * \brief CollectionAnnotationsAttribute
     */
    CollectionAnnotationsAttribute();
    /*!
     * \brief CollectionAnnotationsAttribute
     * \param annotations
     */
    explicit CollectionAnnotationsAttribute(const QMap<QByteArray, QByteArray> &annotations);
    /*!
     */
    ~CollectionAnnotationsAttribute() override = default;

    /*!
     */
    void setAnnotations(const QMap<QByteArray, QByteArray> &annotations);
    /*!
     */
    [[nodiscard]] QMap<QByteArray, QByteArray> annotations() const;

    /*!
     */
    [[nodiscard]] QByteArray type() const override;
    /*!
     */
    CollectionAnnotationsAttribute *clone() const override;
    /*!
     */
    [[nodiscard]] QByteArray serialized() const override;
    /*!
     */
    void deserialize(const QByteArray &data) override;

    /*!
     */
    bool operator==(const CollectionAnnotationsAttribute &other) const;

private:
    QMap<QByteArray, QByteArray> mAnnotations;
};

}
