/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_SEARCHRESULTJOB_H
#define AKONADI_SEARCHRESULTJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class SearchResultJobPrivate;
class ImapSet;
class Collection;

class AKONADICORE_EXPORT SearchResultJob : public Akonadi::Job
{
    Q_OBJECT
public:
    explicit SearchResultJob(const QByteArray &searchId, const Collection &collection, QObject *parent = 0);
    virtual ~SearchResultJob();

    void setSearchId(const QByteArray &searchId);
    QByteArray searchId() const;

    void setResult(const ImapSet &set);
    void setResult(const QVector<QByteArray> &remoteIds);
    void setResult(const QVector<qint64> &ids);

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(SearchResultJob)
};
}

#endif // AKONADI_SEARCHRESULTJOB_H
