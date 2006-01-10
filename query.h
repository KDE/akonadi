/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#ifndef PIM_QUERY_H
#define PIM_QUERY_H

#include <libakonadi/job.h>

namespace PIM {

/**
  This class queries the pim storage service for a list of
  @see DataReferences, which match a given query pattern.

  Example for synchronous API:

  \code
    Query query;
    query.setQueryPattern( "mimetype=message/rfc822; folder=/inbox; date>=2005-12-31;" );
    query.exec();
    const QList<PIM::DataReference> result = query.result();
  \endcode
 */
class Query : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new Query.
     */
    Query();

    /**
      Destroys the Query.
     */
    virtual ~Query();

    /**
      Sets the query pattern.
     */
    void setQueryPattern( const QString &pattern );

    /**
      Returns a list of @see DataReferences which match the
      query pattern set by @see setQueryPattern().
     */
    QList<PIM::DataReference> result() const;

  protected:
    /**
      Subclasses can reimplement this function to modify the query
      pattern.
     */
    virtual void doSetQueryPattern( const QString &pattern );

  private:
    void doStart();

    class QueryPrivate;
    QueryPrivate *d;
};

}

#endif
