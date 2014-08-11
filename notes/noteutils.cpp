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
#include <quuid.h>
#include <qdom.h>
#include <QTextDocument>

namespace Akonadi {
namespace NoteUtils {

#define X_NOTES_UID_HEADER "X-Akonotes-UID"
#define X_NOTES_LASTMODIFIED_HEADER "X-Akonotes-LastModified"
#define X_NOTES_CLASSIFICATION_HEADER "X-Akonotes-Classification"
#define X_NOTES_CUSTOM_HEADER "X-Akonotes-Custom"

#define CLASSIFICATION_PUBLIC "Public"
#define CLASSIFICATION_PRIVATE "Private"
#define CLASSIFICATION_CONFIDENTIAL "Confidential"

#define X_NOTES_URL_HEADER "X-Akonotes-Url"
#define X_NOTES_LABEL_HEADER "X-Akonotes-Label"
#define X_NOTES_CONTENTTYPE_HEADER "X-Akonotes-Type"
#define CONTENT_TYPE_CUSTOM "custom"
#define CONTENT_TYPE_ATTACHMENT "attachment"

#define ENCODING "utf-8"

class Attachment::AttachmentPrivate
{
  public:
    AttachmentPrivate( const QUrl& url, const QString& mimetype )
    : mUrl( url ),
      mMimetype( mimetype )
    {}

    AttachmentPrivate( const QByteArray& data, const QString& mimetype )
    : mData( data ),
      mMimetype( mimetype )
    {}

    AttachmentPrivate( const AttachmentPrivate &other )
    {
      *this = other;
    }

    QUrl mUrl;
    QByteArray mData;
    QString mMimetype;
    QString mLabel;
};

Attachment::Attachment( const QUrl& url, const QString& mimetype )
: d_ptr( new Attachment::AttachmentPrivate( url, mimetype ) )
{
}

Attachment::Attachment( const QByteArray& data, const QString& mimetype )
: d_ptr( new Attachment::AttachmentPrivate( data, mimetype ) )
{
}

Attachment::Attachment( const Attachment &other )
: d_ptr(new AttachmentPrivate(*other.d_func()) )
{

}

Attachment::~Attachment()
{
  delete d_ptr;
}

bool Attachment::operator==( const Attachment &a ) const
{
  const Q_D( Attachment );
  if ( d->mUrl.isEmpty() ) {
    return d->mUrl == a.d_func()->mUrl &&
    d->mMimetype == a.d_func()->mMimetype &&
    d->mLabel == a.d_func()->mLabel;
  }
  return d->mData == a.d_func()->mData &&
    d->mMimetype == a.d_func()->mMimetype &&
    d->mLabel == a.d_func()->mLabel;
}

void Attachment::operator=( const Attachment &a )
{
  *d_ptr = *a.d_ptr;
}

QUrl Attachment::url() const
{
  const Q_D( Attachment );
  return d->mUrl;
}

QByteArray Attachment::data() const
{
  const Q_D( Attachment );
  return d->mData;
}

QString Attachment::mimetype() const
{
  const Q_D( Attachment );
  return d->mMimetype;
}

void Attachment::setLabel( const QString& label )
{
  Q_D( Attachment );
  d->mLabel = label;
}

QString Attachment::label() const
{
  const Q_D( Attachment );
  return d->mLabel;
}

class NoteMessageWrapper::NoteMessageWrapperPrivate
{
  public:
    NoteMessageWrapperPrivate()
    : classification( Public )
    {
    }

    NoteMessageWrapperPrivate( const KMime::Message::Ptr &msg )
    : textFormat( Qt::PlainText ),
      classification( Public )
    {
      readMimeMessage(msg);
    }

    void readMimeMessage(const KMime::Message::Ptr &msg );

    KMime::Content* createCustomPart() const;
    void parseCustomPart( KMime::Content * );

    KMime::Content* createAttachmentPart( const Attachment & ) const;
    void parseAttachmentPart( KMime::Content * );

    QString uid;
    QString title;
    QString text;
    Qt::TextFormat textFormat;
    QString from;
    KDateTime creationDate;
    KDateTime lastModifiedDate;
    QMap< QString, QString > custom;
    QList<Attachment> attachments;
    Classification classification;
};

void NoteMessageWrapper::NoteMessageWrapperPrivate::readMimeMessage(const KMime::Message::Ptr& msg)
{
  if (!msg.get()) {
    kWarning() << "Empty message";
    return;
  }
  title = msg->subject( true )->asUnicodeString();
  text = msg->mainBodyPart()->decodedText( true ); //remove trailing whitespace, so we get rid of "  " in empty notes
  if ( msg->from( false ) )
    from = msg->from( false )->asUnicodeString();
  creationDate = msg->date( true )->dateTime();
  if ( msg->mainBodyPart()->contentType( false ) && msg->mainBodyPart()->contentType()->mimeType() == "text/html" ) {
    textFormat = Qt::RichText;
  }

  if (KMime::Headers::Base *lastmod = msg->headerByType(X_NOTES_LASTMODIFIED_HEADER)) {
    const QByteArray &s = lastmod->asUnicodeString().toLatin1();
    const char *cursor = s.constData();
    if (!KMime::HeaderParsing::parseDateTime( cursor, cursor + s.length(), lastModifiedDate)) {
      kWarning() << "failed to parse lastModifiedDate";
    }
  }

  if (KMime::Headers::Base *uidHeader = msg->headerByType(X_NOTES_UID_HEADER)) {
    uid = uidHeader->asUnicodeString();
  }

  if (KMime::Headers::Base *classificationHeader = msg->headerByType(X_NOTES_CLASSIFICATION_HEADER)) {
    const QString &c = classificationHeader->asUnicodeString();
    if ( c == CLASSIFICATION_PRIVATE ) {
      classification = Private;
    } else if ( c == CLASSIFICATION_CONFIDENTIAL ) {
      classification = Confidential;
    }
  }

  const KMime::Content::List list = msg->contents();
  Q_FOREACH(KMime::Content *c, msg->contents()) {
    if (KMime::Headers::Base *typeHeader = c->headerByType(X_NOTES_CONTENTTYPE_HEADER)) {
      const QString &type = typeHeader->asUnicodeString();
      if ( type == CONTENT_TYPE_CUSTOM ) {
        parseCustomPart(c);
      } else if ( type == CONTENT_TYPE_ATTACHMENT ) {
        parseAttachmentPart(c);
      } else {
        qWarning() << "unknown type " << type;
      }
    }
  }
}

QDomDocument createXMLDocument()
{
  QDomDocument document;
  QString p = "version=\"1.0\" encoding=\"UTF-8\"";
  document.appendChild(document.createProcessingInstruction( "xml", p ) );
  return document;
}

QDomDocument loadDocument(KMime::Content *part)
{
  QString errorMsg;
  int errorLine, errorColumn;
  QDomDocument document;
  bool ok = document.setContent( part->body(), &errorMsg, &errorLine, &errorColumn );
  if ( !ok ) {
    kWarning() << part->body();
    qWarning( "Error loading document: %s, line %d, column %d", qPrintable( errorMsg ), errorLine, errorColumn );
    return QDomDocument();
  }
  return document;
}

KMime::Content* NoteMessageWrapper::NoteMessageWrapperPrivate::createCustomPart() const
{
  KMime::Content* content = new KMime::Content();
  content->appendHeader( new KMime::Headers::Generic( X_NOTES_CONTENTTYPE_HEADER, content, CONTENT_TYPE_CUSTOM, ENCODING ) );
  QDomDocument document = createXMLDocument();
  QDomElement element = document.createElement( "custom" );
  element.setAttribute( "version", "1.0" );
  for ( QMap <QString, QString >::const_iterator it = custom.begin(); it != custom.end(); ++it ) {
    QDomElement e = element.ownerDocument().createElement( it.key() );
    QDomText t = element.ownerDocument().createTextNode( it.value() );
    e.appendChild( t );
    element.appendChild( e );
    document.appendChild( element );
  }
  content->setBody( document.toString().toLatin1() );
  return content;
}

void NoteMessageWrapper::NoteMessageWrapperPrivate::parseCustomPart( KMime::Content* part )
{
  QDomDocument document = loadDocument( part );
  if (document.isNull()) {
    return;
  }
  QDomElement top = document.documentElement();
  if ( top.tagName() != "custom" ) {
    qWarning( "XML error: Top tag was %s instead of the expected custom",
              top.tagName().toLatin1().data() );
    return;
  }

  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      custom.insert(e.tagName(), e.text());
    } else {
      kDebug() <<"Node is not an element";
      Q_ASSERT(false);
    }
  }
}

KMime::Content* NoteMessageWrapper::NoteMessageWrapperPrivate::createAttachmentPart( const Attachment &a ) const
{
  KMime::Content* content = new KMime::Content();
  content->appendHeader( new KMime::Headers::Generic( X_NOTES_CONTENTTYPE_HEADER, content, CONTENT_TYPE_ATTACHMENT, ENCODING ) );
  if (a.url().isValid()) {
    content->appendHeader( new KMime::Headers::Generic( X_NOTES_URL_HEADER, content, a.url().toString().toLatin1(), ENCODING ) );
  } else {
    content->setBody( a.data() );
  }
  content->contentType()->setMimeType( a.mimetype().toLatin1() );
  if (!a.label().isEmpty()) {
    content->appendHeader( new KMime::Headers::Generic( X_NOTES_LABEL_HEADER, content, a.label().toLatin1(), ENCODING ) );
  }
  content->contentTransferEncoding()->setEncoding( KMime::Headers::CEbase64 );
  content->contentDisposition()->setDisposition( KMime::Headers::CDattachment );
  content->contentDisposition()->setFilename( "attachment" );
  return content;
}

void NoteMessageWrapper::NoteMessageWrapperPrivate::parseAttachmentPart( KMime::Content *part )
{
  QString label;
  if ( KMime::Headers::Base *labelHeader = part->headerByType( X_NOTES_LABEL_HEADER ) ) {
    label = labelHeader->asUnicodeString();
  }
  if ( KMime::Headers::Base *header = part->headerByType( X_NOTES_URL_HEADER ) ) {
    Attachment attachment( QUrl( header->asUnicodeString() ), part->contentType()->mimeType() );
    attachment.setLabel( label );
    attachments.append(attachment);
  } else {
    Attachment attachment( part->decodedContent(), part->contentType()->mimeType() );
    attachment.setLabel( label );
    attachments.append(attachment);
  }
}

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

  KDateTime lastModifiedDate = KDateTime::currentLocalDateTime();
  if ( d->lastModifiedDate.isValid() ) {
    lastModifiedDate = d->lastModifiedDate;
  }

  QString uid;
  if ( !d->uid.isEmpty() ) {
    uid = d->uid;
  } else {
    uid = QUuid::createUuid();
  }

  msg->subject( true )->fromUnicodeString( title, ENCODING );
  msg->date( true )->setDateTime( creationDate );
  msg->from( true )->fromUnicodeString( d->from, ENCODING );
  msg->mainBodyPart()->fromUnicodeString( text );
  msg->mainBodyPart()->contentType( true )->setMimeType( d->textFormat == Qt::RichText ? "text/html" : "text/plain" );
  msg->appendHeader( new KMime::Headers::Generic(X_NOTES_LASTMODIFIED_HEADER, msg.get(), lastModifiedDate.toString( KDateTime::RFCDateDay ).toLatin1(), ENCODING ) );
  msg->appendHeader( new KMime::Headers::Generic( X_NOTES_UID_HEADER, msg.get(), uid, ENCODING ) );

  QString classification = QString::fromLatin1(CLASSIFICATION_PUBLIC);
  switch ( d->classification ) {
    case Private:
      classification = QString::fromLatin1(CLASSIFICATION_PRIVATE);
      break;
    case Confidential:
      classification = QString::fromLatin1(CLASSIFICATION_CONFIDENTIAL);
      break;
    default:
      //do nothing
      break;
  }
  msg->appendHeader( new KMime::Headers::Generic( X_NOTES_CLASSIFICATION_HEADER, msg.get(), classification, ENCODING ) );

  foreach (const Attachment &a, d->attachments) {
    msg->addContent( d->createAttachmentPart(a) );
  }

  if ( !d->custom.isEmpty() ) {
    msg->addContent( d->createCustomPart() );
  }

  msg->assemble();
  return msg;
}

void NoteMessageWrapper::setUid( const QString &uid )
{
  Q_D( NoteMessageWrapper );
  d->uid = uid;
}

QString NoteMessageWrapper::uid() const
{
  const Q_D( NoteMessageWrapper );
  return d->uid;
}

void NoteMessageWrapper::setClassification( NoteMessageWrapper::Classification classification )
{
  Q_D( NoteMessageWrapper );
  d->classification = classification;
}

NoteMessageWrapper::Classification NoteMessageWrapper::classification() const
{
  const Q_D( NoteMessageWrapper );
  return d->classification;
}

void NoteMessageWrapper::setLastModifiedDate( const KDateTime& lastModifiedDate )
{
  Q_D( NoteMessageWrapper );
  d->lastModifiedDate = lastModifiedDate;
}

KDateTime NoteMessageWrapper::lastModifiedDate() const
{
  const Q_D( NoteMessageWrapper );
  return d->lastModifiedDate;
}

void NoteMessageWrapper::setCreationDate( const KDateTime &creationDate )
{
  Q_D( NoteMessageWrapper );
  d->creationDate = creationDate;
}

KDateTime NoteMessageWrapper::creationDate() const
{
  const Q_D( NoteMessageWrapper );
  return d->creationDate;
}

void NoteMessageWrapper::setFrom( const QString &from )
{
  Q_D( NoteMessageWrapper );
  d->from = from;
}

QString NoteMessageWrapper::from() const
{
  const Q_D( NoteMessageWrapper );
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

QList<Attachment> &NoteMessageWrapper::attachments()
{
  Q_D( NoteMessageWrapper );
  return d->attachments;
}

QMap< QString, QString > &NoteMessageWrapper::custom()
{
  Q_D( NoteMessageWrapper );
  return d->custom;
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
