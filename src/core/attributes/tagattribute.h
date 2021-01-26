/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGATTRIBUTE_H
#define AKONADI_TAGATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <QColor>

#include <memory>

namespace Akonadi
{
/**
 * @short Attribute that stores the properties that are used to display a tag.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT TagAttribute : public Attribute
{
public:
    explicit TagAttribute();

    ~TagAttribute() override;

    /**
     * Sets the @p name that should be used for display.
     */
    void setDisplayName(const QString &name);

    /**
     * Returns the name that should be used for display.
     * Users of this should fall back to Collection::name() if this is empty.
     */
    QString displayName() const;

    /**
     * Sets the icon @p name for the default icon.
     */
    void setIconName(const QString &name);

    /**
     * Returns the icon name of the icon returned by icon().
     */
    QString iconName() const;

    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
    void setTextColor(const QColor &color);
    QColor textColor() const;
    void setFont(const QString &fontKey);
    QString font() const;
    void setInToolbar(bool inToolbar);
    bool inToolbar() const;
    void setShortcut(const QString &shortcut);
    QString shortcut() const;

    /**
     * Sets the priority of the tag.
     * The priority is primarily used for presentation, e.g. for sorting.
     * If only one tag can be displayed for a given item, the one with the highest
     * priority should be shown.
     */
    void setPriority(int priority);

    /**
     * Returns the priority of the tag.
     * The default value is -1
     */
    int priority() const;

    /* reimpl */
    QByteArray type() const override;
    TagAttribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    TagAttribute(const TagAttribute &other);
    TagAttribute &operator=(const TagAttribute &other);
    //@cond PRIVATE
    class Private;
    const std::unique_ptr<Private> d;
    //@endcond
};

} // namespace Akonadi

#endif
