/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "imapset.h"

#include "imapparser.h"

#include <QtCore/QSharedData>

using namespace Akonadi;

class ImapInterval::Private : public QSharedData
{
  public:
    Private() :
      QSharedData(),
      begin( 0 ),
      end( 0 )
    {}

    Private( const Private &other ) :
      QSharedData( other )
    {
      begin = other.begin;
      end = other.end;
    }

    int begin;
    int end;
};

class ImapSet::Private : public QSharedData
{
  public:
    Private() : QSharedData() {}
    Private( const Private &other ) :
      QSharedData( other )
    {
    }

    ImapInterval::List intervals;
};


ImapInterval::ImapInterval() :
    d( new Private )
{
}

ImapInterval::ImapInterval(const ImapInterval & other) :
    d( other.d )
{
}

ImapInterval::ImapInterval(int begin, int end) :
    d( new Private )
{
  d->begin = begin;
  d->end = end;
}

ImapInterval::~ ImapInterval()
{
}

ImapInterval& ImapInterval::operator =(const ImapInterval & other)
{
  if ( this != & other )
    d = other.d;
  return *this;
}

bool ImapInterval::operator ==(const ImapInterval & other) const
{
  return ( d->begin == other.d->begin && d->end == other.d->end );
}

int ImapInterval::size() const
{
  if ( !d->begin && !d->end )
    return 0;
  return d->end - d->begin + 1;
}

bool ImapInterval::hasDefinedBegin() const
{
  return d->begin != 0;
}

int ImapInterval::begin() const
{
  return d->begin;
}

bool ImapInterval::hasDefinedEnd() const
{
  return d->end != 0;
}

int ImapInterval::end() const
{
  if ( hasDefinedEnd() )
    return d->end;
  return 0xFFFFFFFF; // should be INT_MAX, but where is that defined again?
}

void ImapInterval::setBegin(int value)
{
  Q_ASSERT( value >= 0 );
  Q_ASSERT( value <= d->end || !hasDefinedEnd() );
  d->begin = value;
}

void ImapInterval::setEnd(int value)
{
  Q_ASSERT( value >= 0 );
  Q_ASSERT( value >= d->begin || !hasDefinedBegin() );
  d->end = value;
}

QByteArray Akonadi::ImapInterval::toImapSequence() const
{
  if ( size() == 0 )
    return QByteArray();
  if ( size() == 1 )
    return QByteArray::number( d->begin );
  QByteArray rv;
  rv += QByteArray::number( d->begin ) + ":";
  if ( hasDefinedEnd() )
    rv += QByteArray::number( d->end );
  else
    rv += "*";
  return rv;
}


ImapSet::ImapSet() :
    d( new Private )
{
}

ImapSet::ImapSet(const ImapSet & other) :
    d( other.d )
{
}

ImapSet::~ImapSet()
{
}

ImapSet & ImapSet::operator =(const ImapSet & other)
{
  if ( this != &other )
    d = other.d;
  return *this;
}

void ImapSet::add(const QList< int > & values)
{
  QList<int> vals = values;
  qSort( vals );
  for( int i = 0; i < vals.count(); ++i ) {
    const int begin = vals[i];
    Q_ASSERT( begin >= 0 );
    if ( i == vals.count() - 1 ) {
      d->intervals << ImapInterval( begin, begin );
      break;
    }
    do {
      ++i;
      Q_ASSERT( vals[i] >= 0 );
      if ( vals[i] != (vals[i - 1] + 1) ) {
        --i;
        break;
      }
    } while ( i < vals.count() - 1 );
    d->intervals << ImapInterval( begin, vals[i] );
  }
}

void ImapSet::add(const ImapInterval & interval)
{
  d->intervals << interval;
}

QByteArray ImapSet::toImapSequenceSet() const
{
  QList<QByteArray> rv;
  foreach ( const ImapInterval interval, d->intervals )
    rv << interval.toImapSequence();
  return ImapParser::join( rv, "," );
}

ImapInterval::List ImapSet::intervals() const
{
  return d->intervals;
}

bool ImapSet::isEmpty() const
{
  return d->intervals.isEmpty();
}

QDebug& operator<<( QDebug &d, const Akonadi::ImapInterval &interval )
{
  d << interval.toImapSequence();
  return d;
}

