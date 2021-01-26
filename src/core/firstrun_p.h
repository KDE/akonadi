/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FIRSTRUN_P_H
#define AKONADI_FIRSTRUN_P_H

#include <QObject>
#include <QStringList>
#include <QVariant>

class KConfig;
class KJob;
struct QMetaObject;

namespace Akonadi
{
/**
 Takes care of setting up default resource agents when running Akonadi for the first time.

 <h4>Defining your own default agent setups</h4>

  To add an additional agent to the default Akonadi setup, add a file with the
  agent setup description into <QStandardPaths::GenericDataLocation>/akonadi/firstrun.

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
  <li>Name: The user visible name for this agent (only used for resource agents currently)</li>
  </ul>

  The [Settings] group is optional and contains agent-dependent settings.
  For those settings to be applied, the agent needs to export its settings
  via D-Bus using the KConfigXT &lt;-&gt; D-Bus bridge.
*/
class Firstrun : public QObject
{
    Q_OBJECT
public:
    explicit Firstrun(QObject *parent = nullptr);
    ~Firstrun();

private:
    void findPendingDefaults();
    void setupNext();
    static QVariant::Type argumentType(const QMetaObject *mo, const QString &method);

private Q_SLOTS:
    void instanceCreated(KJob *job);

private:
    QStringList mPendingDefaults;
    KConfig *const mConfig;
    KConfig *mCurrentDefault = nullptr;
};

}

#endif
