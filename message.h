/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef PIM_MESSAGE_H
#define PIM_MESSAGE_H

#include <libakonadi/job.h>

namespace KMime {
  class Message;
}

namespace PIM {

/**
  Representation of an email message or a news article.
*/
class AKONADI_EXPORT Message
{
  public:
    typedef QList<Message*> List;

    /**
      Creates a new Message object.

      @param ref The data reference of this message.
    */
    Message( const DataReference &ref = DataReference() );

    /**
      Delete this message.
    */
    ~Message();

    /**
      Returns the DataReference of this object.
    */
    DataReference reference() const;

    /**
      Returns the actual content of this message, including message headers
      (as far as available).
    */
    KMime::Message* mime() const;

    /**
      Sets the message content. The Message object takes the ownership of
      @p mime, ie. it will take care of deleting it.
    */
    void setMime( KMime::Message* mime );

  private:
    class Private;
    Private* d;
};

}

#endif
