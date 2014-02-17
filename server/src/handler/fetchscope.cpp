/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "fetchscope.h"
#include "imapstreamparser.h"
#include "protocol_p.h"
#include "handlerhelper.h"
#include "handler.h"

using namespace Akonadi::Server;

class FetchScope::Private : public QSharedData
{
  public:
    Private();
    Private( const Private &other );

    void parseCommandStream();
    void parsePartList();

    ImapStreamParser *mStreamParser;

    QVector<QByteArray> mRequestedParts;
    QStringList mRequestedPayloads;
    QDateTime mChangedSince;

    int mAncestorDepth;
    uint mCacheOnly : 1;
    uint mCheckCachedPayloadPartsOnly : 1;
    uint mFullPayload : 1;
    uint mAllAttrs : 1;
    uint mSizeRequested : 1;
    uint mMTimeRequested : 1;
    uint mExternalPayloadSupported : 1;
    uint mRemoteRevisionRequested : 1;
    uint mIgnoreErrors : 1;
    uint mFlagsRequested : 1;
    uint mRemoteIdRequested : 1;
    uint mGidRequested : 1;
    uint mTagsRequested : 1;
};

FetchScope::Private::Private()
  : QSharedData()
  , mStreamParser( 0 )
  , mAncestorDepth( 0 )
  , mCacheOnly( false )
  , mCheckCachedPayloadPartsOnly( false )
  , mFullPayload( false )
  , mAllAttrs( false )
  , mSizeRequested( false )
  , mMTimeRequested( false )
  , mExternalPayloadSupported( false )
  , mRemoteRevisionRequested( false )
  , mIgnoreErrors( false )
  , mFlagsRequested( false )
  , mRemoteIdRequested( false )
  , mGidRequested( false )
  , mTagsRequested( false )
{
}

FetchScope::Private::Private( const Private &other )
  : QSharedData( other )
  , mStreamParser( other.mStreamParser )
  , mRequestedParts( other.mRequestedParts )
  , mRequestedPayloads( other.mRequestedPayloads )
  , mChangedSince( other.mChangedSince )
  , mAncestorDepth( other.mAncestorDepth )
  , mCacheOnly( other.mCacheOnly )
  , mCheckCachedPayloadPartsOnly( other.mCheckCachedPayloadPartsOnly )
  , mFullPayload( other.mFullPayload )
  , mAllAttrs( other.mAllAttrs )
  , mSizeRequested( other.mSizeRequested )
  , mMTimeRequested( other.mMTimeRequested )
  , mExternalPayloadSupported( other.mExternalPayloadSupported )
  , mRemoteRevisionRequested( other.mRemoteRevisionRequested )
  , mIgnoreErrors( other.mIgnoreErrors )
  , mFlagsRequested( other.mFlagsRequested )
  , mRemoteIdRequested( other.mRemoteIdRequested )
  , mGidRequested( other.mGidRequested )
  , mTagsRequested( other.mTagsRequested )
{
}


void FetchScope::Private::parseCommandStream()
{
  Q_ASSERT( mStreamParser );

  // macro vs. attribute list
  Q_FOREVER {
    if ( mStreamParser->atCommandEnd() ) {
      break;
    }
    if ( mStreamParser->hasList() ) {
      parsePartList();
      break;
    } else {
      const QByteArray buffer = mStreamParser->readString();
      if ( buffer == AKONADI_PARAM_CACHEONLY ) {
        mCacheOnly = true;
      } else if ( buffer == AKONADI_PARAM_CHECKCACHEDPARTSONLY ) {
        mCheckCachedPayloadPartsOnly = true;
      } else if ( buffer == AKONADI_PARAM_ALLATTRIBUTES ) {
        mAllAttrs = true;
      } else if ( buffer == AKONADI_PARAM_EXTERNALPAYLOAD ) {
        mExternalPayloadSupported = true;
      } else if ( buffer == AKONADI_PARAM_FULLPAYLOAD ) {
        mRequestedParts << AKONADI_PARAM_PLD_RFC822; // HACK: temporary workaround until we have support for detecting the availability of the full payload in the server
        mFullPayload = true;
      } else if ( buffer == AKONADI_PARAM_ANCESTORS ) {
        mAncestorDepth = HandlerHelper::parseDepth( mStreamParser->readString() );
      } else if ( buffer == AKONADI_PARAM_IGNOREERRORS ) {
        mIgnoreErrors = true;
      } else if ( buffer == AKONADI_PARAM_CHANGEDSINCE ) {
        bool ok = false;
        mChangedSince = QDateTime::fromTime_t( mStreamParser->readNumber( &ok ) );
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
  mStreamParser->beginList();
  while ( !mStreamParser->atListEnd() ) {
    const QByteArray b = mStreamParser->readString();
    // filter out non-part attributes we send all the time
    if ( b == AKONADI_PARAM_REVISION || b == AKONADI_PARAM_UID ) {
      continue;
    } else if ( b == AKONADI_PARAM_REMOTEID ) {
      mRemoteIdRequested = true;
    } else if ( b == AKONADI_PARAM_FLAGS ) {
      mFlagsRequested = true;
    } else if ( b == AKONADI_PARAM_SIZE ) {
      mSizeRequested = true;
    } else if ( b == AKONADI_PARAM_MTIME ) {
      mMTimeRequested = true;
    } else if ( b == AKONADI_PARAM_REMOTEREVISION ) {
      mRemoteRevisionRequested = true;
    } else if ( b == AKONADI_PARAM_GID ) {
      mGidRequested = true;
    } else if ( b == AKONADI_PARAM_TAGS ) {
      mTagsRequested = true;
    } else if ( b == AKONADI_PARAM_COLLECTIONID ) {
      // we always return collection IDs anyway
    } else {
      mRequestedParts.push_back( b );
      if ( b.startsWith( AKONADI_PARAM_PLD ) ) {
        mRequestedPayloads.push_back( QString::fromLatin1( b ) );
      }
    }
  }
}

FetchScope::FetchScope()
  : d( new Private )
{
}

FetchScope::FetchScope( ImapStreamParser *streamParser )
  : d( new Private )
{
  d->mStreamParser = streamParser;
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
  if ( this != &other ) {
    d = other.d;
  }

  return *this;
}

bool FetchScope::isValid() const
{
  return d->mStreamParser != 0;
}

ImapStreamParser *FetchScope::streamParser() const
{
  return d->mStreamParser;
}

QVector<QByteArray> FetchScope::requestedParts() const
{
  return d->mRequestedParts;
}

QStringList FetchScope::requestedPayloads() const
{
  return d->mRequestedPayloads;
}

QDateTime FetchScope::changedSince() const
{
  return d->mChangedSince;
}

int FetchScope::ancestorDepth() const
{
  return d->mAncestorDepth;
}

bool FetchScope::cacheOnly() const
{
  return d->mCacheOnly;
}

bool FetchScope::checkCachedPayloadPartsOnly() const
{
  return d->mCheckCachedPayloadPartsOnly;
}

bool FetchScope::fullPayload() const
{
  return d->mFullPayload;
}

bool FetchScope::allAttributes() const
{
  return d->mAllAttrs;
}

bool FetchScope::sizeRequested() const
{
  return d->mSizeRequested;
}

bool FetchScope::mTimeRequested() const
{
  return d->mMTimeRequested;
}

bool FetchScope::externalPayloadSupported() const
{
  return d->mExternalPayloadSupported;
}

bool FetchScope::remoteRevisionRequested() const
{
  return d->mRemoteRevisionRequested;
}

bool FetchScope::ignoreErrors() const
{
  return d->mIgnoreErrors;
}

bool FetchScope::flagsRequested() const
{
  return d->mFlagsRequested;
}

bool FetchScope::remoteIdRequested() const
{
  return d->mRemoteIdRequested;
}

bool FetchScope::gidRequested() const
{
  return d->mGidRequested;
}

bool FetchScope::tagsRequested() const
{
  return d->mTagsRequested;
}
