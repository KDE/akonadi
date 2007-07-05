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

#ifndef AKONADI_XESAMMANAGER_H
#define AKONADI_XESAMMANAGER_H

#include <QMutex>
#include <QObject>

#include "entities.h"

class OrgFreedesktopXesamSearchInterface;

namespace Akonadi {

class XesamManager : public QObject
{
  Q_OBJECT
  public:
    XesamManager( QObject* parent = 0 );
    ~XesamManager();

    static XesamManager* instance() { return mInstance; }
    bool addSearch( const Location &loc );
    bool removeSearch( int loc );

  private:
    void reloadSearches();
    void stopSearches();
    int uriToItemId( const QString &uri );

  private slots:
    void slotHitsAdded( const QString &search, int count );
    void slotHitsRemoved( const QString &search, const QList<int> &hits );
    void slotHitsModified( const QString &search, const QList<int> &hits );

  private:
    OrgFreedesktopXesamSearchInterface *mInterface;
    QString mSession;
    QHash<QString,int> mSearchMap;
    QHash<int,QString> mInvSearchMap;
    static XesamManager* mInstance;
    QMutex mMutex;
};

}

#endif
