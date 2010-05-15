/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_PERSISTENTSEARCHATTRIBUTE_H
#define AKONADI_PERSISTENTSEARCHATTRIBUTE_H

#include <akonadi/attribute.h>

namespace Akonadi {

/**
 * Query properties of persistent search collections.
 * @since 4.5
 */
//AK_REVIEW: extend API doc with usage example
class AKONADI_EXPORT PersistentSearchAttribute : public Akonadi::Attribute
{
  public:
    /**
     * Creates a new persistent search attribute.
     */
    PersistentSearchAttribute();

    /**
     * Destroys the persistent search attribute.
     */
    ~PersistentSearchAttribute();

    /**
     * Returns the query language used for this search.
     */
    QString queryLanguage() const;

    /**
     * Sets the query language used for this search.
     * @param language the query language
     */
    void setQueryLanguage( const QString &language );

    /**
     * Returns the query string used for this search.
     */
    QString queryString() const;

    /**
     * Sets the query string to be used for this search.
     * @param query The query string.
     */
    void setQueryString( const QString &query );

    //@cond PRIVATE
    virtual QByteArray type() const;
    virtual Attribute *clone() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );
    //@endcond

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
