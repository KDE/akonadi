/*
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#ifndef BENCHMARKER_H
#define BENCHMARKER_H

#include <QtCore/QTime>

#include <libakonadi/agentmanager.h>
#include <libakonadi/job.h>

using namespace Akonadi;

class BenchMarker : public QObject
{
  Q_OBJECT
  public:
    BenchMarker( const QString &maildir );

  private Q_SLOTS:
    QString createAgent( const QString &name );
    void agentInstanceAdded( const QString &instance );
    void agentInstanceRemoved( const QString &instance );
    void agentInstanceProgressChanged( const QString &agentIdentifier, uint progress, const QString &message );
    void agentInstanceStatusChanged( const QString &instance, AgentManager::Status status, const QString &message );
    void outputStats( const QString &description );
    void output( const QString &message );
    void stop();

  private:
    void testMaildir( QString dir );

    QString currentInstance;
    QString currentAccount;
    QTime timer;
    bool done;
    AgentManager *manager;
};

#endif
