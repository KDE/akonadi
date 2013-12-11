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

#ifndef AKONADI_AGENTSEARCHREQUEST_H
#define AKONADI_AGENTSEARCHREQUEST_H

#include <QObject>
#include <QVector>
#include <QStringList>

namespace Akonadi
{

class AkonadiConnection;

class AgentSearchRequest: public QObject
{
    Q_OBJECT

  public:
    AgentSearchRequest( AkonadiConnection *connection );
    ~AgentSearchRequest();

    void setQuery( const QString &query );
    QString query() const;
    void setCollections( const QVector<qint64> &collections );
    QVector<qint64> collections() const;
    void setMimeTypes( const QStringList &mimeTypes );
    QStringList mimeTypes() const;

    AkonadiConnection *connection() const;

    void exec();

  Q_SIGNALS:
    void resultsAvailable( const QSet<qint64> &results );

  private:
    AkonadiConnection *mConnection;
    QString mQuery;
    QVector<qint64> mCollections;
    QStringList mMimeTypes;

};

}

#endif // AKONADI_AGENTSEARCHREQUEST_H
