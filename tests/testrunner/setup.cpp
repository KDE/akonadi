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
  QDBusInterface  agentDBus( "org.freedesktop.Akonadi.Control", "/AgentManager",
                           "org.freedesktop.Akonadi.AgentManager", *mInternalBus );

  kDebug() << "available agent types:" << agentDBus.call( QLatin1String( "agentTypes" ) );

  kDebug() << config->getAgents();
  foreach(const QString &agent, config->getAgents()){
    kDebug() << "inserting resource:"<<agent;
    kDebug() << agentDBus.call( QLatin1String( "createAgentInstance" ), agent);
  }
}

void SetupTest::dbusNameOwnerChanged( const QString &name, const QString &oldOwner, const QString &newOwner )
{
  kDebug() << name << oldOwner << newOwner;

  // TODO: find out why it does not work properly when reacting on
  // org.freedesktop.Akonadi.Control
  if ( name == QLatin1String( "org.freedesktop.Akonadi" ) )
    setupAgents();
}

SetupTest::SetupTest()
{

  clearEnvironment();

  Config *config = Config::getInstance();

  setenv("KDEHOME", config->getKdeHome().toAscii() , 1 );
  setenv("XDG_DATA_HOME", config->getXdgDataHome().toAscii() , 1 );
  setenv("XDG_CONFIG_HOME", config->getXdgConfigHome().toAscii() , 1 );

  Symbols *symbol = Symbols::getInstance();
  symbol->insertSymbol("KDEHOME", config->getKdeHome());
  symbol->insertSymbol("XDG_DATA_HOME", config->getXdgDataHome());
  symbol->insertSymbol("XDG_CONFIG_HOME", config->getXdgConfigHome());

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

  delete akonadiDaemonProcess;
}
