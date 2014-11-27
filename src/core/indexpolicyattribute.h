/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_INDEXPOLICYATTRIBUTE_H
#define AKONADI_INDEXPOLICYATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi {

/**
 * @short An attribute to specify how a collection should be indexed for searching.
 *
 * This attribute can be attached to any collection and should be honored by indexing
 * agents.
 *
 * @since 4.6
 */
class AKONADICORE_EXPORT IndexPolicyAttribute : public Akonadi::Attribute
{
public:
    /**
     * Creates a new index policy attribute.
     */
    IndexPolicyAttribute();

    /**
     * Destroys the index policy attribute.
     */
    ~IndexPolicyAttribute();

    /**
     * Returns whether this collection is supposed to be indexed at all.
     */
    bool indexingEnabled() const;

    /**
     * Sets whether this collection should be indexed at all.
     * @param enable @c true to enable indexing, @c false to exclude this collection from indexing
     */
    void setIndexingEnabled(bool enable);

    //@cond PRIVATE
    virtual QByteArray type() const Q_DECL_OVERRIDE;
    virtual Attribute *clone() const Q_DECL_OVERRIDE;
    virtual QByteArray serialized() const Q_DECL_OVERRIDE;
    virtual void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;
    //@endcond

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
