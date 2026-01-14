/*
 *  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "akonadicore_export.h"

#include "attribute.h"

#include <QColor>

namespace Akonadi
{
/**
 * @short Attribute that stores colors of a collection.
 *
 * Storing color in Akonadi makes it possible to sync them between client and server.
 *
 * @author Sandro Knauß <knauss@kolabsys.com>
 * @since 5.3
 */

class AKONADICORE_EXPORT CollectionColorAttribute : public Akonadi::Attribute
{
public:
    /*!
     */
    explicit CollectionColorAttribute() = default;
    /*!
     */
    explicit CollectionColorAttribute(const QColor &color);

    /*!
     */
    ~CollectionColorAttribute() override = default;

    /*!
     */
    void setColor(const QColor &color);
    /*!
     */
    [[nodiscard]] QColor color() const;

    /*!
     */
    [[nodiscard]] QByteArray type() const override;
    /*!
     */
    CollectionColorAttribute *clone() const override;
    /*!
     */
    [[nodiscard]] QByteArray serialized() const override;
    /*!
     */
    void deserialize(const QByteArray &data) override;

private:
    QColor mColor;
};

} // namespace Akonadi
