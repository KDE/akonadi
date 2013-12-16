/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include "qsflphonedialer.h"
#include "../dbusconnectionpool.h"

#include <KLocalizedString>

#include <QDBusConnectionInterface>

#include <QtCore/QProcess>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QDebug>

#include <unistd.h>

static bool isSflPhoneServiceRegistered()
{
    const QLatin1String service( "org.sflphone.SFLphone" );

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

QSflPhoneDialer::QSflPhoneDialer( const QString &applicationName )
    : QDialer( applicationName )
{
}

QSflPhoneDialer::~QSflPhoneDialer()
{
}

bool QSflPhoneDialer::initializeSflPhone()
{
    // first check whether dbus interface is available yet
    if ( !isSflPhoneServiceRegistered() ) {

        // it could be skype is not running yet, so start it now
        if ( !QProcess::startDetached( QLatin1String( "sflphone-client-kde" ), QStringList() ) ) {
            mErrorMessage = i18n( "Unable to start sflphone-client-kde process, check that sflphone-client-kde executable is in your PATH variable." );
            return false;
        }

        const int runs = 100;
        for ( int i = 0; i < runs; ++i ) {
            if ( !isSflPhoneServiceRegistered() ) {
                ::sleep( 2 );
            } else {
                return true;
            }
        }
    }
    return true;
}

bool QSflPhoneDialer::dialNumber(const QString &number)
{
    if ( !initializeSflPhone() ) {
        return false;
    }

    QStringList arguments;
    arguments << QLatin1String("--place-call");
    arguments << number;
    if (!QProcess::startDetached( QLatin1String( "sflphone-client-kde" ), arguments)) {
        return false;
    }

    return true;
}

bool QSflPhoneDialer::sendSms(const QString &number, const QString &text)
{
    if ( !initializeSflPhone() ) {
        return false;
    }

    QStringList arguments;
    arguments << QLatin1String("--send-text");
    arguments << number;
    arguments << QLatin1String("--message");
    arguments << text;
    if (!QProcess::startDetached( QLatin1String( "sflphone-client-kde" ), arguments)) {
        return false;
    }
    return true;
}
