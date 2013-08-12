/**
  This file is part of the akonadi-calendar library.

  Copyright (C) 2013 Sérgio Martins <iamsergio@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef ICALIMPORTER_P_H
#define ICALIMPORTER_P_H

#include "icalimporter.h"
#include "incidencechanger.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <QString>
#include <QObject>
#include <QList>

class KJob;
class KTemporaryFile;
class QByteArray;
namespace KIO {
    class Job;
}

namespace Akonadi {

class ICalImporter::Private : public QObject {
    Q_OBJECT
public:
    Private(Akonadi::IncidenceChanger *changer, ICalImporter *qq);
    ~Private();
    void setErrorMessage(const QString &);

    ICalImporter *const q;
    Akonadi::IncidenceChanger *m_changer;
    int m_numIncidences;
    int m_numErrors;
    QList<int> m_pendingRequests;

    QString m_lastErrorMessage;
    bool m_working;
    KTemporaryFile *m_temporaryFile;
    Akonadi::Collection m_collection;
public Q_SLOTS:
    void resourceCreated(KJob *);
    void remoteDownloadFinished(KIO::Job *, const QByteArray &);
    void onIncidenceCreated(int changeId,
                            const Akonadi::Item &item,
                            Akonadi::IncidenceChanger::ResultCode resultCode,
                            const QString &errorString);
};

}
#endif // ICALIMPORTER_P_H
