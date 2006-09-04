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

#include <resourcebase.h>

class QTimer;

namespace PIM {

class KnutResource : public ResourceBase
{
  Q_OBJECT

  public:
    KnutResource( const QString &id );
    ~KnutResource();

  public Q_SLOTS:
    virtual bool requestItemDelivery( const QString&, const QString&, const QString&, int );

    virtual void configure();
    virtual bool setConfiguration( const QString& );
    virtual QString configuration() const;

    virtual void synchronize();

  private Q_SLOTS:
    void statusTimeout();
    void syncTimeout();

  private:
    QString mConfig;
    QTimer *mStatusTimer;
    QTimer *mSyncTimer;
};

}

#endif
