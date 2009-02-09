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

#ifndef DBCONFIG_H
#define DBCONFIG_H

#include <QtSql/QSqlDatabase>

/**
 * Access to the database configuration.
 */
namespace DbConfig {

  /**
   * Call before first use. Loads the config,
   * provides usable defaults if needed.
   */
  void init();

  /**
   * Configure the given database object.
   */
  void configure( QSqlDatabase &db );

  /**
   * Returns the active database driver name.
   */
  QString driverName();

  /**
   * Returns wether an internal server should be used.
   */
  bool useInternalServer();

  /**
   * Returns the path to the executable of the internal server.
   */
  QString serverPath();

  /**
   * Returns the database name.
   */
  QString databaseName();

  /**
   * Payload data bigger than this value will be stored in separate files, instead of the database. Valid
   * only if @ref useExternalPayloadFile returns true, otherwise it is ignored.
   * @return the size threshold in bytes, defaults to 4096.
   */
  qint64 sizeThreshold();

  /**
   * Check if big payload data (@ref sizeThreshold) is stored in external files instead of the database.
   * @return true, if the big data is stored in external files. Default is false.
   */
  bool useExternalPayloadFile();

}

#endif
