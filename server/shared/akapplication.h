/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKAPPLICATION_H
#define AKAPPLICATION_H

#include <QtCore/QCoreApplication>

#include <boost/program_options.hpp>

/**
 * D-Bus session bus monitoring and command line handling.
 */
class AkApplication : public QCoreApplication
{
  Q_OBJECT
  public:
    AkApplication( int & argc, char ** argv );
    void parseCommandLine();
    void setDescription( const QString &desc ) { mDescription = desc; }

    void addCommandLineOptions( const boost::program_options::options_description &desc );
    const boost::program_options::variables_map& commandLineArguments() const { return mCmdLineArguments; }

    void printUsage() const;

  private slots:
    void pollSessionBus() const;

  private:
    int mArgc;
    char **mArgv;
    QString mDescription;

    boost::program_options::options_description mCmdLineOptions;
    boost::program_options::variables_map mCmdLineArguments;
};

#endif
