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

#ifndef AKONADI_FETCHSCOPE_H
#define AKONADI_FETCHSCOPE_H

#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include <QStringList>

namespace Akonadi {
namespace Server {

class ImapStreamParser;

class FetchScope
{
  public:
    FetchScope();
    FetchScope( ImapStreamParser *streamParser );
    FetchScope( const FetchScope &other );
    ~FetchScope();

    FetchScope &operator=( const FetchScope &other );

    bool isValid() const;

    ImapStreamParser *streamParser() const;

    void setRequestedParts( const QVector<QByteArray> &requestedParts );
    QVector<QByteArray> requestedParts() const;
    void setRequestedPayloads( const QStringList &payloads );
    QStringList requestedPayloads() const;
    void setChangedSince( const QDateTime &dt );
    QDateTime changedSince() const;
    void setAncestorDepth( int depth );
    int ancestorDepth() const;
    void setCacheOnly( bool cacheOnly );
    bool cacheOnly() const;
    void setCheckCachedPayloadPartsOnly( bool checkCachedPayloadPartsOnly );
    bool checkCachedPayloadPartsOnly() const;
    void setFullPayload( bool fullPayload );
    bool fullPayload() const;
    void setAllAttributes( bool allAttributes );
    bool allAttributes() const;
    void setSizeRequested( bool sizeRequested );
    bool sizeRequested() const;
    void setMTimeRequested( bool mTimeRequested );
    bool mTimeRequested() const;
    void setExternalPayloadSupported( bool externalPayloadSupported );
    bool externalPayloadSupported() const;
    void setRemoteRevisionRequested( bool remoteRevisionRequested );
    bool remoteRevisionRequested() const;
    void setIgnoreErrors( bool ignoreErrors );
    bool ignoreErrors() const;
    void setFlagsRequested( bool flagsRequested );
    bool flagsRequested() const;
    void setRemoteIdRequested( bool remoteIdRequested );
    bool remoteIdRequested() const;
    void setGidRequested( bool gidRequested );
    bool gidRequested() const;
    void setTagsRequested( bool tagsRequested );
    bool tagsRequested() const;
    QVector<QByteArray> tagFetchScope() const;
    void setVirtualReferencesRequested( bool vRefRequested );
    bool virtualReferencesRequested() const;

  private:
    class Private;
    QSharedDataPointer<Private> d;

};

} // namespace Server
} // namespace Akonadi

#endif //AKONADI_FETCHSCOPE_H
