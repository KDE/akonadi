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

#ifndef AKONADIFETCHQUERY_H
#define AKONADIFETCHQUERY_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

namespace Akonadi {

/**
 * An tool class which does the parsing of a fetch request for us.
 */
class FetchQuery
{
  public:

    enum Type
    {
      AllType,
      FullType,
      FastType,
      AttributeType,
      AttributeListType
    };

    class Attribute
    {
      public:
        enum Type
        {
          Flags,
          InternalDate,
          RFC822,
          RFC822_Header,
          RFC822_Size,
          RFC822_Text,
          Body,
          Body_Structure,
          Custom
        };

        bool parse( const QByteArray &attribute );

        Type type() const;
        QString name() const { return mName; }

        void dump() const;

      private:
        Type mType;
        QString mName;
    };

    bool parse( const QByteArray &query );

    bool hasAttributeType( Attribute::Type type ) const;
    QList<QByteArray> sequences() const;
    QList<Attribute> attributes() const;
    Type type() const;
    bool isUidFetch() const;

    void dump() const;

  private:
    QList<QByteArray> mSequences;
    QList<Attribute> mAttributes;
    Type mType;
    bool mIsUidFetch;
};

}

#endif
