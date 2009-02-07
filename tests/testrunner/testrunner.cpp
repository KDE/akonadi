/*
 * Copyright (c) 2009  Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "testrunner.h"

#include <KDebug>
#include <KProcess>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

TestRunner::TestRunner(const QStringList& args, QObject* parent)
  : QObject( parent ),
    mArguments( args ),
    mExitCode( 0 ),
    mProcess( 0 )
{
}

int TestRunner::exitCode() const
{
  return mExitCode;
}

void TestRunner::run()
{
  kDebug() << mArguments;
  mProcess = new KProcess( this );
  mProcess->setProgram( mArguments );
  connect( mProcess, SIGNAL( finished( int ) ), SLOT( processFinished( int ) ) );
  // environment setup seems to have been done by setuptest globally already
  mProcess->start();
  if ( !mProcess->waitForStarted() ) {
    kWarning() << mArguments << "failed to start!";
    mExitCode = 255;
    emit finished();
  }
}

void TestRunner::processFinished( int exitCode )
{
  kDebug() << exitCode;
  mExitCode = exitCode;
  emit finished();
}

void TestRunner::terminate()
{
  if ( mProcess )
    mProcess->terminate();
}


#include "testrunner.moc"
