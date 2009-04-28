/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_RESOURCESELECTJOB_H
#define AKONADI_RESOURCESELECTJOB_H

#include "akonadiprivate_export.h"

#include <akonadi/job.h>

namespace Akonadi {

class ResourceSelectJobPrivate;

/**
 * @internal
 *
 * @short Job that selects a resource context for remote identifier based operations.
 *
 * This job selects a resource context that is used whenever remote identifier
 * based operations ( e.g. fetch items or collections by remote identifier ) are
 * executed.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_TESTS_EXPORT ResourceSelectJob : public Job
{
  Q_OBJECT
  public:
    /**
     * Selects the specified resource for all following remote identifier
     * based operations in the same session.
     *
     * @param identifier The resource identifier.
     * @param parent The parent object.
     */
    explicit ResourceSelectJob( const QString &identifier, QObject *parent = 0 );

  protected:
    void doStart();

  private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( ResourceSelectJob )
    //@endcond PRIVATE
};

}

#endif
