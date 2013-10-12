/***************************************************************************
 *   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>                 *
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

#include "fetchscope.h"

#include "imapstreamparser.h"
#include "libs/protocol_p.h"
#include "handler.h"
#include "handlerhelper.h"

#include <QStringList>

using namespace Akonadi;

class FetchScope::Private: public QSharedData
{
  public:
    Private();
    Private( const Private &other );

    void parseCommandStream();
    void parsePartList();

    QVector<QByteArray> requestedParts;
    QStringList requestedPayloads;
    int ancestorDepth;
    bool cacheOnly;
    bool changedOnly;
    bool checkCachedPayloadPartsOnly;
    bool fullPayload;
    bool allAttrs;
    bool sizeRequested;
    bool mTimeRequested;
    bool externalPayloadSupported;
    bool remoteRevisionRequested;
    bool ignoreErrors;
    bool flagsRequested;
    bool remoteIdRequested;
    bool gidRequested;
    QDateTime changedSince;

    ImapStreamParser *streamParser;
};

FetchScope::Private::Private()
 : QSharedData()
 , ancestorDepth( 0 )
 , cacheOnly( false )
 , changedOnly( false )
 , checkCachedPayloadPartsOnly( false )
 , fullPayload( false )
 , allAttrs( false )
 , sizeRequested( false )
 , mTimeRequested( false )
 , externalPayloadSupported( false )
 , remoteRevisionRequested( false )
 , ignoreErrors( false )
 , flagsRequested( false )
 , remoteIdRequested( false )
 , gidRequested( false )
 , streamParser( 0 )
{
}

FetchScope::Private::Private( const Private &other )
 : QSharedData( other )
 , ancestorDepth( other.ancestorDepth )
 , cacheOnly( other.cacheOnly )
 , checkCachedPayloadPartsOnly( other.checkCachedPayloadPartsOnly )
 , fullPayload( other.fullPayload )
 , allAttrs( other.allAttrs )
 , sizeRequested( other.sizeRequested )
 , mTimeRequested( other.mTimeRequested )
 , externalPayloadSupported( other.externalPayloadSupported )
 , remoteRevisionRequested( other.remoteRevisionRequested )
 , ignoreErrors( other.ignoreErrors )
 , flagsRequested( other.flagsRequested )
 , remoteIdRequested( other.remoteIdRequested )
 , gidRequested( other.gidRequested )
 , streamParser( other.streamParser )
{
}

void FetchScope::Private::parseCommandStream()
{
  Q_ASSERT( streamParser );

  // macro vs. attribute list
  Q_FOREVER {
    if ( streamParser->atCommandEnd() ) {
      break;
    }
    if ( streamParser->hasList() ) {
      parsePartList();
      break;
    } else {
      const QByteArray buffer = streamParser->readString();
      if ( buffer == AKONADI_PARAM_CACHEONLY ) {
        cacheOnly = true;
      } else if ( buffer == AKONADI_PARAM_CHECKCACHEDPARTSONLY ) {
        checkCachedPayloadPartsOnly = true;
      } else if ( buffer == AKONADI_PARAM_ALLATTRIBUTES ) {
        allAttrs = true;
      } else if ( buffer == AKONADI_PARAM_EXTERNALPAYLOAD ) {
        externalPayloadSupported = true;
      } else if ( buffer == AKONADI_PARAM_FULLPAYLOAD ) {
        requestedParts << AKONADI_PARAM_PLD_RFC822; // HACK: temporary workaround until we have support for detecting the availability of the full payload in the server
        fullPayload = true;
      } else if ( buffer == AKONADI_PARAM_ANCESTORS ) {
        ancestorDepth = HandlerHelper::parseDepth( streamParser->readString() );
      } else if ( buffer == AKONADI_PARAM_IGNOREERRORS ) {
        ignoreErrors = true;
      } else if ( buffer == AKONADI_PARAM_CHANGEDSINCE ) {
        bool ok = false;
        changedSince = QDateTime::fromTime_t( streamParser->readNumber( &ok ) );
        if ( !ok ) {
          throw HandlerException( "Invalid CHANGEDSINCE timestamp" );
        }
      } else {
        throw HandlerException( "Invalid command argument" );
      }
    }
  }
}

void FetchScope::Private::parsePartList()
{
  streamParser->beginList();
  while ( !streamParser->atListEnd() ) {
    const QByteArray b = streamParser->readString();
    // filter out non-part attributes we send all the time
    if ( b == AKONADI_PARAM_REVISION || b == AKONADI_PARAM_UID ) {
      continue;
    } else if ( b == AKONADI_PARAM_CHANGEDONLY ) {
      changedOnly = true;
    } else if ( b == AKONADI_PARAM_REMOTEID ) {
      remoteIdRequested = true;
    } else if ( b == AKONADI_PARAM_FLAGS ) {
      flagsRequested = true;
    } else if ( b == AKONADI_PARAM_SIZE ) {
      sizeRequested = true;
    } else if ( b == AKONADI_PARAM_MTIME ) {
      mTimeRequested = true;
    } else if ( b == AKONADI_PARAM_REMOTEREVISION ) {
      remoteRevisionRequested = true;
    } else if ( b == AKONADI_PARAM_GID ) {
      gidRequested = true;
    } else {
      requestedParts.push_back( b );
      if ( b.startsWith( AKONADI_PARAM_PLD ) ) {
        requestedPayloads.push_back( QString::fromLatin1( b ) );
      }
    }
  }
}

FetchScope::FetchScope()
 : d( new Private )
{
}

FetchScope::FetchScope( ImapStreamParser *parser )
  : d( new Private )
{
  d->streamParser = parser;
  d->parseCommandStream();
}

FetchScope::FetchScope( const FetchScope &other )
  : d( other.d )
{
}

FetchScope::~FetchScope()
{
}

FetchScope &FetchScope::operator=( const FetchScope &other )
{
  if ( this == &other ) {
    return *this;
  }

  d = other.d;
  return *this;
}

QVector<QByteArray> FetchScope::requestedParts() const
{
  return d->requestedParts;
}

void FetchScope::setRequestedParts( const QVector<QByteArray> &parts )
{
  d->requestedParts = parts;
}

QStringList FetchScope::requestedPayloads() const
{
  return d->requestedPayloads;
}

void FetchScope::setRequestedPayloads( const QStringList &payloads )
{
  d->requestedPayloads = payloads;
}

int FetchScope::ancestorDepth() const
{
  return d->ancestorDepth;
}

void FetchScope::setAncestorDepth( int depth )
{
  d->ancestorDepth = depth;
}

bool FetchScope::cacheOnly() const
{
  return d->cacheOnly;
}

void FetchScope::setCacheOnly( bool cacheOnly )
{
  d->cacheOnly = cacheOnly;
}

bool FetchScope::changedOnly() const
{
  return d->changedOnly;
}

void FetchScope::setChangedOnly( bool changedOnly )
{
  d->changedOnly = changedOnly;
}

bool FetchScope::checkCachedPayloadPartsOnly() const
{
  return d->checkCachedPayloadPartsOnly;
}

void FetchScope::setCheckCachedPayloadPartsOnly( bool cachedOnly )
{
  d->checkCachedPayloadPartsOnly = cachedOnly;
}

bool FetchScope::fullPayload() const
{
  return d->fullPayload;
}

void FetchScope::setFullPayload( bool fullPayload )
{
  d->fullPayload = fullPayload;
}

bool FetchScope::allAttrs() const
{
  return d->allAttrs;
}

void FetchScope::setAllAttrs( bool allAttrs )
{
  d->allAttrs = allAttrs;
}

bool FetchScope::sizeRequested()
{
  return d->sizeRequested;
}

void FetchScope::setSizeRequested( bool sizeRequested )
{
  d->sizeRequested = sizeRequested;
}

bool FetchScope::mTimeRequested() const
{
  return d->mTimeRequested;
}

void FetchScope::setMTimeRequested( bool mTimeRequested )
{
  d->mTimeRequested = mTimeRequested;
}

bool FetchScope::externalPayloadSupported() const
{
  return d->externalPayloadSupported;
}

void FetchScope::setExternalPayloadSupported( bool supported )
{
  d->externalPayloadSupported = supported;
}

bool FetchScope::remoteRevisionRequested() const
{
  return d->remoteRevisionRequested;
}

void FetchScope::setRemoteRevisionRequested( bool requested )
{
  d->remoteRevisionRequested = requested;
}

bool FetchScope::ignoreErrors() const
{
  return d->ignoreErrors;
}

void FetchScope::setIgnoreErrors( bool ignoreErrors )
{
  d->ignoreErrors = ignoreErrors;
}

bool FetchScope::flagsRequested() const
{
  return d->flagsRequested;
}

void FetchScope::setFlagsRequested( bool flagsRequested )
{
  d->flagsRequested = flagsRequested;
}

bool FetchScope::remoteIdRequested() const
{
  return d->remoteIdRequested;
}

void FetchScope::setRemoteIdRequested( bool requested )
{
  d->remoteIdRequested = requested;
}

bool FetchScope::gidRequested() const
{
  return d->gidRequested;
}

void FetchScope::setGidRequested( bool requested )
{
  d->gidRequested = requested;
}

QDateTime FetchScope::changedSince() const
{
  return d->changedSince;
}

void FetchScope::setChangedSince( const QDateTime &changedSince )
{
  d->changedSince = changedSince;
}
