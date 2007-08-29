/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SEARCHPROVIDERMANAGER_H
#define AKONADI_SEARCHPROVIDERMANAGER_H

#include "searchproviderinterface.h"
#include "tracerinterface.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>

class QDir;

namespace Akonadi {

class ProcessControl;

/**
  This class keeps track of available search provider.
*/
class SearchProviderManager : public QObject
{
  Q_OBJECT

  public:
    /**
      Create a new search provider manager.
      @param parent The parent object.
    */
    SearchProviderManager( QObject *parent = 0 );

    ~SearchProviderManager();
  public Q_SLOTS:
    /**
      Returns a list of search providers suitable for the given mimetype.
      @param mimeType The mimetype.
    */
    QStringList providersForMimeType( const QString &mimeType ) const;

  private:
    /**
      Returns the path list where the .desktop files for searchproviders are located.
    */
    static QStringList providerInfoPathList();

    /**
      Reads information about available search providers.
    */
    void readProviderInfos();

    /**
      Reads information about available search providers from a given directory.

      @param directory the directory to get provider information from
    */
    void readProviderInfos( const QDir &directory );

    /**
      Start search provider processes.
    */
    void startProviders();

  private slots:
    void providerRegistered( const QString &name, const QString&, const QString &newOwner );

  private:
    /**
      Internal class to store status of available search providers.
    */
    class ProviderInfo
    {
      public:
        QString identifier;
        QStringList mimeTypes;
        QString exec;
        ProcessControl *controller;
        org::kde::Akonadi::SearchProvider *interface;
    };

    QHash<QString,ProviderInfo> mProviderInfos;
    org::kde::Akonadi::Tracer *mTracer;

};

}

#endif
