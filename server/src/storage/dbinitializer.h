/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef DBINITIALIZER_H
#define DBINITIALIZER_H

#include <QtCore/QString>
#include <QtSql/QSqlDatabase>

#include <boost/shared_ptr.hpp>

class QDomElement;

/**
 * A helper class which takes a reference to a database object and
 * the file name of a template file and initializes the database
 * according to the rules in the template file.
 */
class DbInitializer
{
  public:
   typedef boost::shared_ptr<DbInitializer> Ptr;

    /**
      Returns an initializer instance for a given backend.
    */
    static DbInitializer::Ptr createInstance( const QSqlDatabase &database, const QString &templateFile );

    /**
     * Destroys the database initializer.
     */
    virtual ~DbInitializer();

    /**
     * Starts the initialization process.
     * On success true is returned, false otherwise.
     *
     * If something went wrong @see errorMsg() can be used to retrieve more
     * information.
     */
    bool run();

    /**
     * Returns the textual description of an occurred error.
     */
    QString errorMsg() const;

  protected:
    /**
     * Creates a new database initializer.
     *
     * @param database The reference to the database.
     * @param templateFile The template file.
     */
    DbInitializer( const QSqlDatabase &database, const QString &templateFile );

    /** Overwrite in backend-specific sub-classes to return the SQL type for a given C++ type. */
    virtual QString sqlType( const QString &type ) const;
    /** Overwrite in backend-specific sub-classes to return the SQL value for a given C++ value. */
    virtual QString sqlValue( const QString &type, const QString &value ) const;

  private:
    bool checkTable( const QDomElement& );
    bool checkRelation( const QDomElement &element );

    bool hasTable( const QString &tableName );
    bool hasIndex( const QString &tableName, const QString &indexName );

    QSqlDatabase mDatabase;
    QString mTemplateFile;
    QString mErrorMsg;
};

#endif
