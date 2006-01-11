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

#ifndef PIM_DATAREQUEST_H
#define PIM_DATAREQUEST_H

#include <QStringList>

#include <libakonadi/job.h>

namespace PIM {

/**
  This class requests the actual data from the pim storage
  service for a list of @see DataReferences. The request can
  be parameterized by the part of the data your are interested in
  and the acceptable mimetypes.

  Example:

  \code
    SomeDataRequest request( references, "ALL", QStringList( "text/x-vcard" ) );
    if ( !request.exec() ) {
      qDebug( "Error: %s", qPrintable( request.errorString() ) );
      return;
    }

    for ( int i = 0; i < request.count(); ++i ) {
      if ( request.mimetype( i ) == "text/x-vcard" ) {
        doSomeConvertion( request.data( i ) );
      }
    }
  \endcode
 */
class DataRequest : public Job
{
  Q_OBJECT

  public:
    /**
      Creates a new DataRequest for a list of @see DataReferences.

      @param references A list of @see DataReferences.
      @param part The parts of the data which shall be returned.
      @param acceptableMimeTypes A list of acceptable mimetypes.
     */
    DataRequest( const DataReference::List &references, const QString &part = "ALL",
                 const QStringList &acceptableMimeTypes = QStringList() );

    /**
      Destroys the DataRequest.
     */
    ~DataRequest();

    /**
      Returns the numbers of data items.
     */
    int count() const;

    /**
      Returns the plain data of the data item at the given @param index,
      or an empty @see QByteArray if @param index is >= @see count().
     */
    QByteArray data( int index = 0 ) const;

    /**
      Returns the mime type of the data item at the given @param index,
      or an empty @see QByteArray if @param index is >= @see count().
     */
    QByteArray mimeType( int index = 0 ) const;

  private:
    void doStart();

    class DataRequestPrivate;
    DataRequestPrivate *d;
};

}

#endif
