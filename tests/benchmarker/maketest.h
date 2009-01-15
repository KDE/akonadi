/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on kdepimlibs/akonadi/tests/benchmarker.cpp wrote by Robert Zwerus <arzie@dds.nl>

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

#ifndef MAKETEST_H
#define MAKETEST_H

#include <akonadi/agentmanager.h>
#include <akonadi/job.h>

#include <QTime>

class MakeTest: public QObject {

  Q_OBJECT
  protected Q_SLOTS:
    void createAgent( const QString &name );
    void configureDBusIface(const QString &name, const QString &dir );
    void instanceRemoved( const Akonadi::AgentInstance &instance );
    void instanceStatusChanged( const Akonadi::AgentInstance &instance );
    void outputStats( const QString &description );
    void output( const QString &message );

  protected:
    Akonadi::AgentInstance currentInstance;
    QString currentAccount;
    QTime timer;
    bool done;
    void removeCollections();
    void removeResource();
    virtual void runTest()=0;
  public:
    MakeTest();
    void start();
};

#endif
