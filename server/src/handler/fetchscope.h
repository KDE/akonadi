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

    QVector<QByteArray> requestedParts() const;
    QStringList requestedPayloads() const;
    QDateTime changedSince() const;
    int ancestorDepth() const;
    bool cacheOnly() const;
    bool checkCachedPayloadPartsOnly() const;
    bool fullPayload() const;
    bool allAttributes() const;
    bool sizeRequested() const;
    bool mTimeRequested() const;
    bool externalPayloadSupported() const;
    bool remoteRevisionRequested() const;
    bool ignoreErrors() const;
    bool flagsRequested() const;
    bool remoteIdRequested() const;
    bool gidRequested() const;
    bool tagsRequested() const;

  private:
    class Private;
    QSharedDataPointer<Private> d;

};

}

#endif //AKONADI_FETCHSCOPE_H
