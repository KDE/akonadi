/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <QColor>
#include <QIcon>

#include <memory>

namespace Akonadi
{
class EntityDisplayAttributePrivate;

/*!
 * \brief Attribute that stores the properties that are used to display an entity.
 *
 * Display properties of a collection or item, such as translated names and icons.
 *
 * \class Akonadi::EntityDisplayAttribute
 * \inheaderfile Akonadi/EntityDisplayAttribute
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 * \since 4.2
 */
class AKONADICORE_EXPORT EntityDisplayAttribute : public Attribute
{
public:
    /*!
     * Creates a new entity display attribute.
     */
    explicit EntityDisplayAttribute();

    /*!
     * Destroys the entity display attribute.
     */
    ~EntityDisplayAttribute() override;

    /*!
     * Sets the \a name that should be used for display.
     */
    void setDisplayName(const QString &name);

    /*!
     * Returns the name that should be used for display.
     * Users of this should fall back to Collection::name() if this is empty.
     */
    [[nodiscard]] QString displayName() const;

    /*!
     * Sets the icon \a name for the default icon.
     */
    void setIconName(const QString &name);

    /*!
     * Returns the icon that should be used for this collection or item.
     */
    [[nodiscard]] QIcon icon() const;

    /*!
     * Returns the icon name of the icon returned by icon().
     */
    [[nodiscard]] QString iconName() const;

    /*!
     * Sets the icon \a name for the active icon.
     * \a name the icon name to use
     * \since 4.4
     */
    void setActiveIconName(const QString &name);

    /*!
     * Returns the icon that should be used for this collection or item when active.
     * \since 4.4
     */
    [[nodiscard]] QIcon activeIcon() const;

    /*!
     * Returns the icon name of an active item.
     * \since 4.4
     */
    [[nodiscard]] QString activeIconName() const;

    /*!
     * Returns the backgroundColor or an invalid color if none is set.
     * \since 4.4
     */
    [[nodiscard]] QColor backgroundColor() const;

    /*!
     * Sets the backgroundColor to \a color.
     * \a color the background color to use
     * \since 4.4
     */
    void setBackgroundColor(const QColor &color);

    /* reimpl */
    [[nodiscard]] QByteArray type() const override;
    EntityDisplayAttribute *clone() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    const std::unique_ptr<EntityDisplayAttributePrivate> d;
};

} // namespace Akonadi
