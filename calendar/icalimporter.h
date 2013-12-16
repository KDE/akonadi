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

#ifndef ICALIMPORTER_H
#define ICALIMPORTER_H

#include "akonadi-calendar_export.h"

#include <akonadi/collection.h>

#include <QObject>
#include <QString>

/**
 * A class to import ical calendar files into akonadi.
 *
 * @since 4.12
 */
namespace Akonadi {

class IncidenceChanger;

class AKONADI_CALENDAR_EXPORT ICalImporter : public QObject
{
    Q_OBJECT
public:

    /**
     * Constructs a new ICalImporter object. Use a different ICalImporter instance for each file you want to import.
     *
     * @param changer An existing IncidenceChanger, if 0, an internal one will be created.
     *                If you pass an existing one, you will be able to undo/redo import operations.
     * @param parent  Parent QObject.
     */
    explicit ICalImporter(Akonadi::IncidenceChanger *changer = 0, QObject *parent = 0);

    /**
     * Translated error message.
     * This is set when either importIntoExistingFinished() or importIntoNewResource() return false
     * or when the corresponding signals are have success=false.
     */
    QString errorMessage() const;

Q_SIGNALS:
    /**
     * Emitted after calling importIntoExistingResource()
     * @param success Success of the operation.
     * @param total Number of incidences included in the ical file.
     *
     * @see importIntoExistingResource(), errorMessage().
     */
    void importIntoExistingFinished(bool success, int total);

    /**
     * Emitted after calling importIntoNewResource().
     * If success is false, check errorMessage().
     */
    void importIntoNewFinished(bool success);

public Q_SLOTS:

    /**
     * Creates a new akonadi_ical_resource and configures it to use @p filename.
     * @param filename ical absolute file path
     * @return True if the job was started, in this case you should wait for the corresponding signal.
     */
    bool importIntoNewResource(const QString &filename);

    /**
     * Imports an ical file into an existing resource.
     * @param url Path of a local or remote file to import.
     * @param collectionId The destination collection. If null, the user will be prompted for a destination.
     *
     * @return false if some basic validation happened, like insufficient permissions. Use errorMessage() to see
     *         what happened. The importIntoExistingFinished() signal won't be emitted in this case.
     *
     *         true if the import job was started. importIntoExistingFinished() signal will be emitted in this case.
     */
    bool importIntoExistingResource(const QUrl &url, Collection collection);

private:
    class Private;
    Private *const d;
};

}

#endif // ICALIMPORTER_H
