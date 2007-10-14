/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_CHANGERECORDER_H
#define AKONADI_CHANGERECORDER_H

#include <libakonadi/monitor.h>

class QSettings;

namespace Akonadi {

class ChangeRecorderPrivate;

/**
  @internal
  Records and replays change notification.

  @todo Export only for unit tests!
*/
class AKONADI_EXPORT ChangeRecorder : public Monitor
{
  Q_OBJECT
  public:
    /**
      Creates a new change recorder.
    */
    explicit ChangeRecorder( QObject *parent = 0 );

    /**
      Destroys this ChangeRecorder object and writes all not yet processed changes
      to the config file.
    */
    ~ChangeRecorder();

    /**
      Set the QSettings object used for persisting recorded changes.
    */
    void setConfig( QSettings *settings );

    /**
      Checks if there recorded changes.
    */
    bool isEmpty() const;

    /**
      Remove the previously emitted change from the records.
    */
    void changeProcessed();

  public Q_SLOTS:
    /**
      Replay the next change notification and erase the previous one from the record.
    */
    void replayNext();

  Q_SIGNALS:
    /**
      Emitted when new changes are recorded.
    */
    void changesAdded();

  private:
    Q_DECLARE_PRIVATE( ChangeRecorder )
};

}

#endif
