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

#ifndef AKONADI_FETCHSCOPE_H
#define AKONADI_FETCHSCOPE_H

#include <QVector>
#include <QDateTime>
#include <QSharedDataPointer>

namespace Akonadi {

class ImapStreamParser;

class FetchScope
{
  public:
    FetchScope();
    FetchScope( ImapStreamParser *parser );
    FetchScope( const FetchScope &other );
    ~FetchScope();

    FetchScope& operator=( const FetchScope &other );

    QVector<QByteArray> requestedParts() const;
    void setRequestedParts( const QVector<QByteArray> &parts );
    QStringList requestedPayloads() const;
    void setRequestedPayloads( const QStringList &payloads );
    int ancestorDepth() const;
    void setAncestorDepth( int depth );
    bool cacheOnly() const;
    void setCacheOnly( bool cacheOnly );
    bool changedOnly() const;
    void setChangedOnly( bool changedOnly );
    bool checkCachedPayloadPartsOnly() const;
    void setCheckCachedPayloadPartsOnly( bool cachedOnly );
    bool fullPayload() const;
    void setFullPayload( bool fullPayload );
    bool allAttrs() const;
    void setAllAttrs( bool allAttrs );
    bool sizeRequested();
    void setSizeRequested( bool sizeRequested );
    bool mTimeRequested() const;
    void setMTimeRequested( bool mTimeRequested );
    bool externalPayloadSupported() const;
    void setExternalPayloadSupported( bool supported );
    bool remoteRevisionRequested() const;
    void setRemoteRevisionRequested( bool requested );
    bool ignoreErrors() const;
    void setIgnoreErrors( bool ignoreErrors );
    bool flagsRequested() const;
    void setFlagsRequested( bool flagsRequested );
    bool remoteIdRequested() const;
    void setRemoteIdRequested( bool requested );
    bool gidRequested() const;
    void setGidRequested( bool requested );
    QDateTime changedSince() const;
    void setChangedSince( const QDateTime &changedSince );

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

#endif // AKONADI_FETCHSCOPE_H
