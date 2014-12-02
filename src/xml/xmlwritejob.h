/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_XMLWRITEJOB_H
#define AKONADI_XMLWRITEJOB_H

#include "akonadi-xml_export.h"
#include "job.h"

namespace Akonadi {

class Collection;
class XmlWriteJobPrivate;

/**
  Serializes a given Akonadi collection into a XML file.
*/
class AKONADI_XML_EXPORT XmlWriteJob : public Job
{
  Q_OBJECT
  public:
    XmlWriteJob( const Collection &root, const QString &fileName, QObject *parent = Q_NULLPTR );
    XmlWriteJob( const QList<Collection> &roots, const QString &fileName, QObject *parent = Q_NULLPTR );
    ~XmlWriteJob();

  protected:
    /* reimpl. */ void doStart() Q_DECL_OVERRIDE;

  private:
    friend class XmlWriteJobPrivate;
    XmlWriteJobPrivate* const d;
    void done();

    Q_PRIVATE_SLOT( d, void collectionFetchResult(KJob*) )
    Q_PRIVATE_SLOT( d, void itemFetchResult(KJob*) )
};

}

#endif
