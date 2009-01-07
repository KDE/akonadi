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

#ifndef SETUP_H
#define SETUP_H

#include <QProcess>
#include <QDBusConnection>

class SetupTest : public QObject
{
  Q_OBJECT
  private:
    QProcess *akonadiDaemonProcess;
    int dpid;
    bool clearEnvironment();
    QMap<QString, QString> getEnvironment();
    QDBusConnection *mInternalBus;
    int addDBusToEnvironment(QIODevice& io);
    int startDBusDaemon();
    void stopDBusDaemon(int dbuspid);
    void registerWithInternalDBus( const QString &address );
    void setupAgents();
    void copyDirectory(const QString &src, const QString &dst);
    void createTempEnvironment();
    void deleteDirectory(const QString &dirName);
    void cleanTempEnvironment();
  private Q_SLOTS:
    void dbusNameOwnerChanged( const QString &name, const QString &oldOwner, const QString &newOwner );

  public:
    void startAkonadiDaemon();
    void stopAkonadiDaemon();
    SetupTest();
    ~SetupTest();
};

#endif
