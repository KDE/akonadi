/*
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

#ifndef AKONADI_RELATIONFETCHJOB_H
#define AKONADI_RELATIONFETCHJOB_H

#include <akonadi/job.h>
#include <akonadi/relation.h>

namespace Akonadi {

class Relation;
class RelationFetchJobPrivate;

/**
 * @short Job that to fetch relations from Akonadi storage.
 * @since 4.15
 */
class AKONADI_EXPORT RelationFetchJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new relation fetch job.
     *
     * @param relation The relation to fetch.
     * @param parent The parent object.
     */
    explicit RelationFetchJob(const Relation &relation, QObject *parent = 0);

    explicit RelationFetchJob(const QStringList &types, QObject *parent = 0);

    void setResource(const QString &identifier);

    /**
     * Returns the relations.
     */
    Relation::List relations() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new relations have been fetched completely.
     *
     * @param relations The fetched relations.
     */
    void relationsReceived(const Akonadi::Relation::List &relations);

protected:
    virtual void doStart();
    virtual void doHandleResponse(const QByteArray &tag, const QByteArray &data);

private:
    Q_DECLARE_PRIVATE(RelationFetchJob)

    //@cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void timeout())
    //@endcond
};

}

#endif
