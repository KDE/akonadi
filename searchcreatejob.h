/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SEARCHCREATEJOB_H
#define AKONADI_SEARCHCREATEJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class SearchCreateJobPrivate;

/**
 * @short Job that creates a virtual/search collection in the Akonadi storage.
 *
 * This job creates so called virtual or search collections, which don't contain
 * real data, but references to items that match a given search query.
 *
 * @code
 *
 * const QString name = "My search folder";
 * const QString query = "...";
 *
 * Akonadi::SearchCreateJob *job = new Akonadi::SearchCreateJob( name, query );
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( jobFinished( KJob* ) ) );
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   qDebug() << "Created search folder successfully";
 *   const Collection searchCollection = job->createdCollection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT SearchCreateJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a search create job.
     *
     * @param name The name of the search collection.
     * @param query The search query (format not defined yet).
     * @param parent The parent object.
     */
    SearchCreateJob( const QString &name, const QString &query, QObject *parent = 0 );

    /**
     * Destroys the search create job.
     */
    ~SearchCreateJob();

    /**
     * Returns the newly created search collection once the job finished successfully. Returns an invalid
     * collection if the job has not yet finished or failed.
     *
     * @since 4.4
     */
    Collection createdCollection() const;

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
    Q_DECLARE_PRIVATE( SearchCreateJob )
};

}

#endif
