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

#ifndef AKONADI_FIRSTRUN_H
#define AKONADI_FIRSTRUN_H

#include <QObject>
#include <QStringList>
#include <QVariant>

class KConfig;
class KJob;
class KProcess;
class QMetaObject;

namespace Akonadi {

/**
 Takes care of setting up default resource agents when running Akonadi for the first time.

 <h4>Defining your own default agent setups</h4>

  To add an additional agent to the default Akonadi setup, add a file with the
  agent setup description into $KDEDIR/share/akonadi/firstrun.

  Such a file looks as follows:

  @verbatim
  [Agent]
  Id=defaultaddressbook
  Type=akonadi_vcard_resource
  Name=My Addressbook

  [Settings]
  Path[$e]=~/.kde/share/apps/kabc/std.ics
  AutosaveInterval=1
  @endverbatim

  The keys in the [Agent] group are mandatory:
  <ul>
  <li>Id: A unique identifier of the setup description, should never change to avoid the agent
  being set up twice.</li>
  <li>Type: The agent type</li>
  <li>Nanme: The user visible name for this agent (only used for resource agents currently)</li>
  </ul>

  The [Settings] group is optional and contains agent-dependent settings.
  For those settings to be applied, the agent needs to export its settings
  via D-Bus using the KConfigXT &lt;-&gt; D-Bus bridge.
*/
class Firstrun : public QObject
{
  Q_OBJECT
  public:
    Firstrun( QObject * parent = 0 );
    ~Firstrun();

  private:
    void findPendingDefaults();
    void setupNext();
    void migrateKresType( const QString &resourceFamily );
    static QVariant::Type argumentType( const QMetaObject *mo, const QString &method );

  private slots:
    void instanceCreated( KJob* job );
    void migrationFinished( int exitCode );

  private:
    QStringList mPendingDefaults;
    KConfig *mConfig;
    KConfig *mCurrentDefault;
    KProcess *mProcess;
    QString mResourceFamily;
};

}

#endif

