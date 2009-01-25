/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
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

#include "setup.h"
#include "config.h"
#include "symbols.h"

#include <kapplication.h>
#include <kdebug.h>

#include <QMap>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <signal.h>
#include <unistd.h>

QMap<QString, QString> SetupTest::getEnvironment()
{
  QMap<QString, QString> env;

  foreach (const QString& val, QProcess::systemEnvironment()) {
    int p = val.indexOf('=');
    if (p > 0) {
      env[val.left(p).toUpper()] = val.mid(p+1);
    }
  }
  return env;
}


bool SetupTest::clearEnvironment()
{
  QMap<QString, QString> environment = getEnvironment();

  foreach(const QString& s, environment.keys()) {
    if (s != "HOME") {
      if ( !unsetenv( s.toAscii() )) {
        return false;
      }
    }
  }

  return true;
}

int SetupTest::addDBusToEnvironment(QIODevice& io) {
  QByteArray data = io.readLine();
  int pid = -1;
  Symbols *symbol = Symbols::getInstance();

  while (data.size()) {
    if (data[data.size()-1] == '\n') {
      data.resize(data.size()-1);
    }
    QString val(data);
    int p = val.indexOf('=');
    if (p > 0) {
      QString name = val.left(p).toUpper();
      val = val.mid(p+1);
      if (name == "DBUS_SESSION_BUS_PID") {
        pid = val.toInt();
        setenv(name.toAscii(), val.toAscii(), 1);
        symbol->insertSymbol(name, val);
      } else if (name == "DBUS_SESSION_BUS_ADDRESS") {
        setenv(name.toAscii(), val.toAscii(), 1);
        symbol->insertSymbol(name, val);
      }
    }
    data = io.readLine();
  }
  return pid;
}

int SetupTest::startDBusDaemon()
{

  QProcess dbusprocess;
  QStringList dbusargs;

  dbusprocess.start("/usr/bin/dbus-launch", dbusargs);
  bool ok = dbusprocess.waitForStarted() && dbusprocess.waitForFinished();
  if (!ok) {
    kDebug() << "error starting dbus-launch";
    dbusprocess.kill();
    return -1;
  }

  int dbuspid = addDBusToEnvironment(dbusprocess);
  return dbuspid;
}

void SetupTest::stopDBusDaemon(int dbuspid)
{
  kDebug() << dbuspid;
  if (dbuspid) kill(dbuspid, 15);
  sleep(1);

  if (dbuspid) kill(dbuspid, 9);
}

void SetupTest::registerWithInternalDBus( const QString &address )
{
  mInternalBus = new QDBusConnection( QDBusConnection::connectToBus( address, QLatin1String( "InternalBus" ) ) );
  mInternalBus->registerService( QLatin1String( "org.kde.Akonaditest" ) );
  mInternalBus->registerObject(QLatin1String("/MainApplication"),
                             KApplication::kApplication(),
                             QDBusConnection::ExportScriptableSlots |
                             QDBusConnection::ExportScriptableProperties |
                             QDBusConnection::ExportAdaptors);

  QDBusConnectionInterface *busInterface = mInternalBus->interface();
  connect( busInterface, SIGNAL( serviceOwnerChanged( QString, QString, QString ) ),
           this, SLOT( dbusNameOwnerChanged( QString, QString, QString ) ) );
}

void SetupTest::startAkonadiDaemon()
{
  QString akonadiDaemon = "akonadi_control";
  QStringList args;
  akonadiDaemonProcess->start(akonadiDaemon, args);
  akonadiDaemonProcess->waitForStarted(5000);
  kDebug()<<akonadiDaemonProcess->pid();
}

void SetupTest::stopAkonadiDaemon()
{
  akonadiDaemonProcess->terminate();
  if (!akonadiDaemonProcess->waitForFinished(5000)) {
    kDebug() << "Problem finishing process.";
  }
  akonadiDaemonProcess->close();
}

void SetupTest::setupAgents()
{
  Config *config = Config::getInstance();
  QDBusInterface agentDBus( "org.freedesktop.Akonadi.Control", "/AgentManager",
                            "org.freedesktop.Akonadi.AgentManager", *mInternalBus );

  kDebug() << "available agent types:" << agentDBus.call( QLatin1String( "agentTypes" ) );

  kDebug() << config->getAgents();
  foreach(const QString &agent, config->getAgents()){
    kDebug() << "inserting resource:" << agent;
    QDBusReply<QString> reply = agentDBus.call( QLatin1String( "createAgentInstance" ), agent );
    if ( reply.isValid() && !reply.value().isEmpty() )
      mPendingAgents << reply.value();
    else
      kError() << "createAgentInstance call failed:" << reply.error();
  }

  if ( mPendingAgents.isEmpty() )
    emit setupDone();
}

void SetupTest::dbusNameOwnerChanged( const QString &name, const QString &oldOwner, const QString &newOwner )
{
  kDebug() << name << oldOwner << newOwner;

  // TODO: find out why it does not work properly when reacting on
  // org.freedesktop.Akonadi.Control
  if ( name == QLatin1String( "org.freedesktop.Akonadi" ) ) {
    setupAgents();
    return;
  }

  if ( name.startsWith( QLatin1String( "org.freedesktop.Akonadi.Agent." ) ) ) {
    const QString identifier = name.mid( 30 );
    if ( mPendingAgents.contains( identifier ) ) {
      mPendingAgents.removeAll( identifier );
      if ( mPendingAgents.isEmpty() )
        emit setupDone();
    }
  }
}

void SetupTest::copyDirectory(const QString &src, const QString &dst)
{
  QDir srcDir(src);
  srcDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);

  QFileInfoList list = srcDir.entryInfoList();
  for (int i = 0; i < list.size(); ++i) {
    if(list.at(i).isDir()){
      QDir tmpDir(dst);
      tmpDir.mkdir(list.at(i).fileName());
      copyDirectory(list.at(i).absoluteFilePath(), dst + "/" + list.at(i).fileName());
    } else {
      QFile::copy(srcDir.absolutePath() + "/" + list.at(i).fileName(), dst + "/" + list.at(i).fileName());
    }
  }
}

void SetupTest::createTempEnvironment()
{
  QDir tmpDir = QDir::temp();
  const QString testRunnerDir = QString("akonadi_testrunner");
  const QString testRunnerKdeHomeDir = testRunnerDir + QString("/kdehome");
  const QString testRunnerDataDir = testRunnerDir + QString("/data");
  const QString testRunnerConfigDir = testRunnerDir + QString("/config");

  if(!tmpDir.exists(testRunnerDir)){
    tmpDir.mkdir(testRunnerDir);
    tmpDir.mkdir(testRunnerKdeHomeDir);
    tmpDir.mkdir(testRunnerConfigDir);
    tmpDir.mkdir(testRunnerDataDir);
  }

  Config *config = Config::getInstance();
  copyDirectory(testRunnerKdeHomeDir, config->getKdeHome());
  copyDirectory(testRunnerConfigDir, config->getXdgConfigHome());
  copyDirectory(testRunnerDataDir, config->getXdgDataHome());
}

void SetupTest::deleteDirectory(const QString &dirName)
{
  Q_ASSERT( dirName.startsWith( QDir::tempPath() ) ); // just to be sure we don't run amok anywhere
  QDir dir(dirName);
  dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);

  QFileInfoList list = dir.entryInfoList();
  for (int i = 0; i < list.size(); ++i) {
    if(list.at(i).isDir()){
      deleteDirectory(list.at(i).absoluteFilePath());
      QDir tmpDir(list.at(i).absoluteDir());
      tmpDir.rmdir(list.at(i).fileName());
    } else {
      QFile::remove(list.at(i).absoluteFilePath());
    }
  }
}

void SetupTest::cleanTempEnvironment()
{
  deleteDirectory(QDir::tempPath() + "/akonadi_testrunner");
}

SetupTest::SetupTest()
{

  clearEnvironment();
  createTempEnvironment();

  setenv("KDEHOME", "/tmp/akonadi_testrunner/kdehome", 1 );
  setenv("XDG_DATA_HOME", "/tmp/akonadi_testrunner/data", 1 );
  setenv("XDG_CONFIG_HOME", "/tmp/akonadi_testrunner/config", 1 );

  Symbols *symbol = Symbols::getInstance();
  symbol->insertSymbol("KDEHOME", QString("/tmp/akonadi_testrunner/kdehome"));
  symbol->insertSymbol("XDG_DATA_HOME", QString("/tmp/akonadi_testrunner/data"));
  symbol->insertSymbol("XDG_CONFIG_HOME", QString("/tmp/akonadi_testrunner/config"));
  symbol->insertSymbol("AKONADI_TESTRUNNER_PID", QString::number( QCoreApplication::instance()->applicationPid() ) );

  dpid = startDBusDaemon();

#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 2))
  const QString dbusAddress = symbol->getSymbols()[ "DBUS_SESSION_BUS_ADDRESS" ];
  registerWithInternalDBus( dbusAddress );
#endif

  akonadiDaemonProcess = new QProcess();
}

SetupTest::~SetupTest()
{
  stopAkonadiDaemon();
  stopDBusDaemon(dpid);
  cleanTempEnvironment();

  delete akonadiDaemonProcess;
}
