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

#ifndef PIM_EMAILQUERY_H
#define PIM_EMAILQUERY_H

#include <libakonadi/query.h>

namespace PIM {

class EMailQuery : public Query
{
  Q_OBJECT

  public:
    /**
      Creates a new EmailQuery.
     */
    EMailQuery();

    /**
      Destroys the EMailQuery.
     */
    virtual ~EMailQuery();

  protected:
    virtual void doSetQueryPattern( const QString &pattern );

  private:
    void doStart();

    class EMailQueryPrivate;
    EMailQueryPrivate *d;
};

}

#endif
