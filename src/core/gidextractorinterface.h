/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GIDEXTRACTORINTERFACE_H
#define GIDEXTRACTORINTERFACE_H

#include <QObject>

namespace Akonadi
{

class Item;

/**
 * @short An interface to extract the GID of an object contained in an akonadi item.
 *
 * @author Christian Mollekopf <mollekopf@kolabsys.com>
 * @since 4.11
 */
class GidExtractorInterface
{
public:
    /**
     * Destructor.
     */
    virtual ~GidExtractorInterface()
    {
    }
    /**
     * Extracts the globally unique id of @p item
     *
     * If you want to clear the gid from the database return QString("").
     */
    virtual QString extractGid(const Item &item) const = 0;

protected:
    explicit GidExtractorInterface() = default;

private:
    Q_DISABLE_COPY_MOVE(GidExtractorInterface)
};

}

Q_DECLARE_INTERFACE(Akonadi::GidExtractorInterface, "org.freedesktop.Akonadi.GidExtractorInterface/1.0")

#endif
