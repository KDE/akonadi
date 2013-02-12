/*
  Copyright (c) 2013 Montel Laurent <montel.org>

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

#include "qekigadialer.h"

#include "../dbusconnectionpool.h"

#include <KLocale>

#include <QDBusConnectionInterface>

#include <QtCore/QProcess>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QDebug>

#include <unistd.h>

static bool isEkigaServiceRegistered()
{
    const QLatin1String service( "org.ekiga.Ekiga" );

    QDBusConnectionInterface *interface = QDBusConnection::systemBus().interface();
    if ( interface->isServiceRegistered( service ) ) {
        return true;
    }

    interface = Akonadi::DBusConnectionPool::threadConnection().interface();
    if ( interface->isServiceRegistered( service ) ) {
        return true;
    }
    return false;
}

QEkigaDialer::QEkigaDialer( const QString &applicationName )
    : QDialer( applicationName )
{
}

QEkigaDialer::~QEkigaDialer()
{
}

bool QEkigaDialer::initializeSflPhone()
{
    // first check whether dbus interface is available yet
    if ( !isEkigaServiceRegistered() ) {

        // it could be skype is not running yet, so start it now
        if ( !QProcess::startDetached( QLatin1String( "ekiga" ), QStringList() ) ) {
            mErrorMessage = i18n( "Unable to start ekiga process, check that ekiga executable is in your PATH variable." );
            return false;
        }

        const int runs = 100;
        for ( int i = 0; i < runs; ++i ) {
            if ( !isEkigaServiceRegistered() ) {
                ::sleep( 2 );
            } else {
                return true;
            }
        }
    }
    return true;
}

bool QEkigaDialer::dialNumber(const QString &number)
{
    if ( !isEkigaServiceRegistered() ) {
        return false;
    }
    //TODO
    return true;
}

bool QEkigaDialer::sendSms(const QString &, const QString &)
{
    mErrorMessage = i18n( "Sending an SMS is currently not supported on Ekiga" );
    return false;
}
