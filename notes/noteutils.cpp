/*  This file is part of the KDE project
    Copyright (C) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "noteutils.h"

#include <klocalizedstring.h>
#include <kdatetime.h>
#include <kmime/kmime_message.h>
#include <kdebug.h>

#include <qstring.h>
#include <qtextdocument.h>

namespace Akonadi {
namespace NoteUtils {

class NoteMessageWrapper::NoteMessageWrapperPrivate
{
  public:
    NoteMessageWrapperPrivate()
    {
    }

    NoteMessageWrapperPrivate( const KMime::Message::Ptr &msg )
    :  textFormat( Qt::PlainText )
    {
      if (msg.get()) {
        title = msg->subject( true )->asUnicodeString();
        text = msg->mainBodyPart()->decodedText( true ); //remove trailing whitespace, so we get rid of "  " in empty notes
        if ( msg->from( false ) )
          from = msg->from( false )->asUnicodeString();
        creationDate = msg->date( true )->dateTime();
        if ( msg->contentType( false ) && msg->contentType( false )->asUnicodeString() == QLatin1String("text/html") ) {
          textFormat = Qt::RichText;
        }
      } else {
        kWarning() << "Empty message";
      }
    }

    QString title;
    QString text;
    QString from;
    KDateTime creationDate;
    Qt::TextFormat textFormat;
};

NoteMessageWrapper::NoteMessageWrapper()
:  d_ptr( new NoteMessageWrapperPrivate() )
{
}

NoteMessageWrapper::NoteMessageWrapper( const KMime::Message::Ptr &msg )
:  d_ptr( new NoteMessageWrapperPrivate(msg) )
{
}

NoteMessageWrapper::~NoteMessageWrapper()
{
  delete d_ptr;
}

KMime::Message::Ptr NoteMessageWrapper::message() const
{
  const Q_D( NoteMessageWrapper );
  KMime::Message::Ptr msg = KMime::Message::Ptr( new KMime::Message() );

  QByteArray encoding( "utf-8" );

  QString title = i18nc( "The default name for new notes.", "New Note" );
  if ( !d->title.isEmpty() ) {
    title = d->title;
  }
  // Need a non-empty body part so that the serializer regards this as a valid message.
  QString text = QLatin1String("  ");
  if ( !d->text.isEmpty() ) {
    text = d->text;
  }

  KDateTime creationDate = KDateTime::currentLocalDateTime();
  if ( d->creationDate.isValid() ) {
    creationDate = d->creationDate;
  }

  msg->subject( true )->fromUnicodeString( title, encoding );
  msg->contentType( true )->setMimeType( d->textFormat == Qt::RichText ? "text/html" : "text/plain" );
  msg->date( true )->setDateTime( creationDate );
  msg->from( true )->fromUnicodeString( d->from, encoding );
  msg->mainBodyPart()->fromUnicodeString( text );

  msg->assemble();
  return msg;
}

void NoteMessageWrapper::setCreationDate( const KDateTime &creationDate )
{
  Q_D(NoteMessageWrapper);
  d->creationDate = creationDate;
}

KDateTime NoteMessageWrapper::creationDate() const
{
  const Q_D(NoteMessageWrapper);
  return d->creationDate;
}

void NoteMessageWrapper::setFrom( const QString &from )
{
  Q_D(NoteMessageWrapper);
  d->from = from;
}

QString NoteMessageWrapper::from() const
{
  const Q_D(NoteMessageWrapper);
  return d->from;
}

void NoteMessageWrapper::setTitle( const QString &title )
{
  Q_D( NoteMessageWrapper );
  d->title = title;
}

QString NoteMessageWrapper::title() const
{
  const Q_D( NoteMessageWrapper );
  return d->title;
}

void NoteMessageWrapper::setText( const QString &text, Qt::TextFormat format )
{
  Q_D( NoteMessageWrapper );
  d->text = text;
  d->textFormat = format;
}

QString NoteMessageWrapper::text() const
{
  const Q_D( NoteMessageWrapper );
  return d->text;
}

Qt::TextFormat NoteMessageWrapper::textFormat() const
{
  const Q_D( NoteMessageWrapper );
  return d->textFormat;
}

QString NoteMessageWrapper::toPlainText() const
{
  const Q_D( NoteMessageWrapper );
  if ( d->textFormat == Qt::PlainText ) {
    return d->text;
  }

  //From cleanHtml in kdepimlibs/kcalutils/incidenceformatter.cpp
  QRegExp rx( QLatin1String("<body[^>]*>(.*)</body>"), Qt::CaseInsensitive );
  rx.indexIn( d->text );
  QString body = rx.cap( 1 );

  return Qt::escape( body.remove( QRegExp( QLatin1String("<[^>]*>") ) ).trimmed() );
}

QString noteIconName()
{
  return QString::fromLatin1( "text-plain" );
}

QString noteMimeType()
{
  return QString::fromLatin1( "text/x-vnd.akonadi.note" );
}

} //End Namepsace
} //End Namepsace
