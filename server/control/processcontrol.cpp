/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "processcontrol.h"

using namespace Akonadi;

ProcessControl::ProcessControl( QObject *parent )
  : QObject( parent ), mFailedToStart( false )
{
  connect( &mProcess, SIGNAL( error( QProcess::ProcessError ) ),
           this, SLOT( slotError( QProcess::ProcessError ) ) );
  connect( &mProcess, SIGNAL( finished( int, QProcess::ExitStatus ) ),
           this, SLOT( slotFinished( int, QProcess::ExitStatus ) ) );
  connect( &mProcess, SIGNAL( readyReadStandardError() ),
           this, SLOT( slotErrorMessages() ) );
}

ProcessControl::~ProcessControl()
{
  if ( mProcess.state() != QProcess::NotRunning )
    mProcess.terminate();
}

void ProcessControl::start( const QString &application, const QStringList &arguments, CrashPolicy policy )
{
  mFailedToStart = false;

  mApplication = application;
  mArguments = arguments;
  mPolicy = policy;

  start();
}

void ProcessControl::slotError( QProcess::ProcessError error )
{
  switch ( error ) {
    case QProcess::Crashed:
      // do nothing, we'll respawn in slotFinished
      break;
    case QProcess::FailedToStart:
    default:
        mFailedToStart = true;
      break;
  }

  qDebug( "ProcessControl: Application '%s' stopped unexpected (%s)",
          qPrintable( mApplication ), qPrintable( mProcess.errorString() ) );
}

void ProcessControl::slotFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
  if ( exitStatus == QProcess::CrashExit ) {
    if ( mPolicy == RestartOnCrash ) {
      if ( !mFailedToStart ) // don't try to start an unstartable application
        start();
    }
  } else {
    if ( exitCode != 0 ) {
      qDebug( "ProcessControl: Application '%s' returned with exit code %d (%s)",
              qPrintable( mApplication ), exitCode, qPrintable( mProcess.errorString() ) );
    }
  }
}

void ProcessControl::slotErrorMessages()
{
  emit processErrorMessages( QString::fromUtf8( mProcess.readAllStandardError() ) );
}

void ProcessControl::start()
{
  mProcess.start( mApplication, mArguments );
  if ( !mProcess.waitForStarted( ) ) {
    qDebug( "ProcessControl: Unable to start application '%s' (%s)",
            qPrintable( mApplication ), qPrintable( mProcess.errorString() ) );
    return;
  }
}

#include "processcontrol.moc"
