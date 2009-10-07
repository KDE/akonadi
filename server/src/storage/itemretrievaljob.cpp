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

#include "itemretrievaljob.h"
#include "itemretrievalrequest.h"

#include <qdbusabstractinterface.h>
#include <qdebug.h>


ItemRetrievalJob::~ItemRetrievalJob()
{
  Q_ASSERT( !m_active );
}


void ItemRetrievalJob::start(QDBusAbstractInterface* interface)
{
  Q_ASSERT( m_request );
  qDebug() << "processing retrieval request for item" << m_request->id << " parts:" << m_request->parts << " of resource:" << m_request->resourceId;

  // call the resource
  if ( interface ) {
    m_active = true;
    QList<QVariant> arguments;
    arguments << m_request->id
              << QString::fromUtf8( m_request->remoteId )
              << QString::fromUtf8( m_request->mimeType )
              << m_request->parts;
    interface->callWithCallback( QLatin1String( "requestItemDelivery" ), arguments, this, SLOT(callFinished(bool)), SLOT(callFailed(QDBusError)) );
  } else {
    emit requestCompleted( m_request, QString::fromLatin1( "Unable to contact resource" ) );
    deleteLater();
  }
}

void ItemRetrievalJob::kill()
{
  m_active = false;
  emit requestCompleted( m_request, QLatin1String( "Request cancelled" ) );
}

void ItemRetrievalJob::callFinished(bool returnValue)
{
  if ( m_active ) {
    m_active = false;
    if ( !returnValue )
      emit requestCompleted( m_request, QString::fromLatin1( "Resource was unable to deliver item" ) );
    else
      emit requestCompleted( m_request, QString() );
  }
  deleteLater();
}

void ItemRetrievalJob::callFailed(const QDBusError& error)
{
  if ( m_active ) {
    m_active = false;
    emit requestCompleted( m_request, QString::fromLatin1( "Unable to retrieve item from resource: %1" ).arg( error.message() ) );
  }
  deleteLater();
}

#include "itemretrievaljob.moc"
