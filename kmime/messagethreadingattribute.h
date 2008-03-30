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

#ifndef AKONADI_MESSAGETHREADINGATTRIBUTE_H
#define AKONADI_MESSAGETHREADINGATTRIBUTE_H

#include "akonadi-kmime_export.h"
#include <akonadi/attribute.h>
#include <akonadi/item.h>

namespace Akonadi {

/**
  Message threading information. Used eg. by MessageThreaderProxyModel
*/
class AKONADI_KMIME_EXPORT MessageThreadingAttribute : public Attribute
{
  public:
    /**
      Creates an empty threading attribute.
    */
    MessageThreadingAttribute();

    /**
      Copy constructor.
    */
    MessageThreadingAttribute( const MessageThreadingAttribute &other );

    /**
      Destructor.
    */
    ~MessageThreadingAttribute();

    /**
      Returns the list of perfect parent message ids.
    */
    QList<Item::Id> perfectParents() const;

    /**
      Sets the list of perfect parent message ids.
    */
    void setPerfectParents( const QList<Item::Id> &parents );

    /**
      Returns the list of non-perfect parent message ids.
    */
    QList<Item::Id> unperfectParents() const;

    /**
      Sets the list of non-perfect parent message ids.
    */
    void setUnperfectParents( const QList<Item::Id> &parents );

    /**
      Returns the list of possible parent message ids based on analyzing the subject.
    */
    QList<Item::Id> subjectParents() const;

    /**
      Sets the list of possible parent message ids based on analyzing the subject.
    */
    void setSubjectParents( const QList<Item::Id> &parents );

    // reimpl.
    QByteArray type() const;
    MessageThreadingAttribute* clone() const;
    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

  private:
    class Private;
    Private * const d;
};

}

#endif
