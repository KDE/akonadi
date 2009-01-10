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

#include <QCoreApplication>
#include <QTimer>

TestRunner::TestRunner(const QStringList& args, QObject* parent) :
  QObject( parent )
{
  kDebug() << args;
  KProcess *process = new KProcess( this );
  process->setProgram( args );
  connect( process, SIGNAL(finished(int)), SLOT(processFinished(int)) );
  // environment setup seems to have been done by setuptest globally already
  process->start();
  if ( !process->waitForStarted() ) {
    kWarning() << args << "failed to start!";
    // QCoreApplication::exit() does not have any effect before the event loop is running, so delay calling it
    QTimer::singleShot( 0, this, SLOT(emitExit()) );
  }
}

 void TestRunner::processFinished(int exitCode)
{
  kDebug() << exitCode;
  QCoreApplication::instance()->exit( exitCode );
}

void TestRunner::emitExit()
{
  QCoreApplication::instance()->exit( 255 );
}

#include "testrunner.moc"
