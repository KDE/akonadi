/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#ifndef KNUTRESOURCE_H
#define KNUTRESOURCE_H

#include <kabc/vcardconverter.h>
#include <kcal/incidence.h>
#include <kcal/icalformat.h>
#include <akonadi/resourcebase.h>

#include <QDomDocument>

class QDomElement;

class KnutResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KnutResource( const QString &id );
    ~KnutResource();

  public Q_SLOTS:
    virtual bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void configure( WId windowId );

    virtual bool setConfiguration( const QString& );
    virtual QString configuration() const;

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &ref );

    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );

  private:
    QDomElement findElementByRid( const QString &rid ) const;

  private:
    class CollectionEntry
    {
      public:
        Akonadi::Collection collection;
        QMap<QString, KABC::Addressee> addressees;
        QMap<QString, KCal::Incidence*> incidences;
    };

    bool loadData();
    void addCollection( const QDomElement &element, const Akonadi::Collection &parentCollection );
    void addAddressee( const QDomElement &element, CollectionEntry &entry );
    void addIncidence( const QDomElement &element, CollectionEntry &entry );

    QString mDataFile;
    KABC::VCardConverter mVCardConverter;
    KCal::ICalFormat mICalConverter;
    QMap<QString, CollectionEntry> mCollections;

    QDomDocument mDocument;
};

#endif
