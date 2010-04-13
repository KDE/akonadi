/*
    Copyright (c) 2010 Till Adam <adam@kde.org>

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

#ifndef AKONADI_SEARCHMODIFYJOB_H
#define AKONADI_SEARCHMODIFYJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class SearchModifyJobPrivate;

/**
 * @short Job that modifies a virtual/search collection in the Akonadi storage.
 *
 * This job updates a virtual or search collections. It should be used when the
 * query underlying the collection changes.
 *
 * @code
 *
 * const Collection existingSearch = ....;
 * const QString query = "...";
 *
 * Akonadi::SearchModifyJob *job = new Akonadi::SearchModifyJob( existingSearch, query );
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( jobFinished( KJob* ) ) );
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   qDebug() << "Modified search folder successfully";
 *   const Collection searchCollection = job->modifiedCollection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Till Adam <adam@kde.org>
 * @since 4.5
 */
class AKONADI_EXPORT SearchModifyJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a search modify job.
     *
     * @param name The name of the search collection.
     * @param query The search query (format not defined yet).
     * @param parent The parent object.
     */
    SearchModifyJob( const Collection &collection, const QString &query, QObject *parent = 0 );

    /**
     * Destroys the search create job.
     */
    ~SearchModifyJob();

    /**
     * Returns the modified search collection once the job finished successfully. Returns an invalid
     * collection if the job has not yet finished or failed.
     *
     */
    Collection modifiedCollection() const;

  protected:
    /**
     * Reimplemented from Akonadi::Job
     */
    void doStart();

    /**
     * Reimplemented from Akonadi::Job
     */
    void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( SearchModifyJob )
};

}

#endif
