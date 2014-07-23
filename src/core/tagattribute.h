/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_TAGATTRIBUTE_H
#define AKONADI_TAGATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <QColor>

namespace Akonadi {

/**
 * @short Attribute that stores the properties that are used to display a tag.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT TagAttribute : public Attribute
{
public:
    TagAttribute();

    ~TagAttribute();

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
    QByteArray type() const;
    TagAttribute *clone() const;
    QByteArray serialized() const;
    void deserialize(const QByteArray &data);

private:
    TagAttribute(const TagAttribute &other);
    TagAttribute &operator=(const TagAttribute &other);
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
