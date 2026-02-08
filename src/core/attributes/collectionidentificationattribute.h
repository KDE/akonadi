/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "attribute.h"
#include <QByteArray>

#include <memory>

namespace Akonadi
{
class CollectionIdentificationAttributePrivate;

/*!
 * \class Akonadi::CollectionIdentificationAttribute
 * \inheaderfile Akonadi/CollectionIdentificationAttribute
 * \inmodule AkonadiCore
 *
 * \brief Attribute that stores additional information on a collection that can be used for searching.
 *
 * Additional indexed properties that can be used for searching.
 *
 * \author Christian Mollekopf <mollekopf@kolabsys.com>
 * \since 4.15
 */
class AKONADICORE_EXPORT CollectionIdentificationAttribute : public Akonadi::Attribute
{
public:
    explicit CollectionIdentificationAttribute(const QByteArray &identifier = QByteArray(),
                                               const QByteArray &folderNamespace = QByteArray(),
                                               const QByteArray &name = QByteArray(),
                                               const QByteArray &organizationUnit = QByteArray(),
                                               const QByteArray &mail = QByteArray());
    ~CollectionIdentificationAttribute() override;

    /*!
     * Sets an identifier for the collection.
     */
    void setIdentifier(const QByteArray &identifier);
    /*!
     */
    [[nodiscard]] QByteArray identifier() const;

    /*!
     */
    void setMail(const QByteArray &);
    /*!
     */
    [[nodiscard]] QByteArray mail() const;

    /*!
     */
    void setOu(const QByteArray &);
    /*!
     */
    [[nodiscard]] QByteArray ou() const;

    /*!
     */
    void setName(const QByteArray &);
    /*!
     */
    [[nodiscard]] QByteArray name() const;

    /*!
     * Sets a namespace the collection is in.
     *
     * Initially used are:
     * * "person" for a collection shared by a person.
     * * "shared" for a collection shared by a person.
     */
    void setCollectionNamespace(const QByteArray &ns);
    /*!
     */
    [[nodiscard]] QByteArray collectionNamespace() const;
    /*!
     */
    [[nodiscard]] QByteArray type() const override;
    /*!
     */
    Attribute *clone() const override;
    /*!
     */
    [[nodiscard]] QByteArray serialized() const override;
    /*!
     */
    void deserialize(const QByteArray &data) override;

private:
    const std::unique_ptr<CollectionIdentificationAttributePrivate> d;
};

} // namespace Akonadi
