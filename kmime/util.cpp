/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "util_p.h"
#include "../dbusconnectionpool.h"
#include "imapsettings.h"
#include "akonadi/servermanager.h"

#include <assert.h>
#include <kio/jobclasses.h>
#include <KDebug>
#include <KIO/JobUiDelegate>

namespace Util
{
/// Helper to sanely show an error message for a job
void showJobError( KJob* job )
{
  assert(job);
  // we can be called from the KJob::kill, where we are no longer a KIO::Job
  // so better safe than sorry
  KIO::Job* kiojob = dynamic_cast<KIO::Job*>(job);
  if ( kiojob && kiojob->ui() )
    kiojob->ui()->showErrorMessage();
  else
    kWarning() << "There is no GUI delegate set for a kjob, and it failed with error:" << job->errorString();
}

OrgKdeAkonadiImapSettingsInterface *createImapSettingsInterface( const QString &ident )
{
  //NOTE(Andras): from kmail/util.cpp
  return new OrgKdeAkonadiImapSettingsInterface( Akonadi::ServerManager::agentServiceName( Akonadi::ServerManager::Resource, ident ),
                                                 QString::fromLatin1("/Settings"),
                                                 Akonadi::DBusConnectionPool::threadConnection() );
}

}
