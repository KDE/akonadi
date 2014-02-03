/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AKONADI_SEARCHRESULTJOB_H
#define AKONADI_SEARCHRESULTJOB_H

#include <akonadi/job.h>
#include <akonadi/private/imapset_p.h>
#include <akonadi/collection.h>

namespace Akonadi {

class SearchResultJobPrivate;

class SearchResultJob : public Akonadi::Job
{
  Q_OBJECT
  public:
    explicit SearchResultJob( const QByteArray &searchId, const Collection &collection, QObject* parent = 0 );
    virtual ~SearchResultJob();

    void setSearchId( const QByteArray &searchId );
    QByteArray searchId() const;

    void setResult( const ImapSet &set );
    void setResult( const QVector<QByteArray> &remoteIds );
    void setResult( const QVector<qint64> &ids );

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( SearchResultJob )
};
}

#endif // AKONADI_SEARCHRESULTJOB_H
