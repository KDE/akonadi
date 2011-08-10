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

#ifndef NOTEUTILS_H
#define NOTEUTILS_H

#include "akonadi-notes_export.h"
#include <QtGui/QTextEdit>

class KDateTime;
class QString;

namespace boost {
  template <typename T> class shared_ptr;
}

namespace KMime {
  class Message;
  typedef boost::shared_ptr<Message> MessagePtr;
}
namespace Akonadi {
namespace NoteUtils {

/**
* @return mimetype for notes
* @since 4.8
*/
AKONADI_NOTES_EXPORT QString noteMimeType();

/**
* @return icon for notes
* @since 4.8
*/
AKONADI_NOTES_EXPORT QString noteIconName();

/**
* A convenience wrapper around KMime::Message::Ptr for notes
*
* This is the format used by the Akonotes Resource
*
* Reading a note from an Akonotes akonadi item:
* @code
* if ( item.hasPayload<KMime::Message::Ptr>() ) {
*   NoteUtils::NoteMessageWrapper note(item.payload<KMime::Message::Ptr>());
*   kDebug() << note.text();
*   textIsRich = messageWrapper.textFormat() == Qt::RichText;
* }
* @endcode
*
* Setting the note as payload of an akonadi Item
* @code
* item.setMimeType(NoteUtils::noteMimeType());
* NoteUtils::NoteMessageWrapper note;
* note.setTitle( "title" );
* note.setText( "text" );
* note.setFrom( QString::fromLatin1( "MyApplication@kde4" ) );
* item.setPayload( note.message() );
* @endcode
*
* @author Christian Mollekopf <chrigi_1@fastmail.fm>
* @since 4.8
*/
class AKONADI_NOTES_EXPORT NoteMessageWrapper
{
public:
  NoteMessageWrapper();
  explicit NoteMessageWrapper( const KMime::MessagePtr & );
  ~NoteMessageWrapper();

  /**
    * Set the title of the note
    */
  void setTitle( const QString &title );

  /**
    * Returns the title of the note
    */
  QString title() const;

  /**
    * Set the text of the note
    *
    * @param format only Qt::PlainText and Qt::RichText is supported
    */
  void setText( const QString &text, Qt::TextFormat format = Qt::PlainText );

  /**
    * Returns the text of the note
    */
  QString text() const;

  /**
    * @return Qt::PlainText or Qt::RichText
    */
  Qt::TextFormat textFormat() const;

  /**
    * @return plaintext version of the text (if richtext)
    */
  QString toPlainText() const;

  /**
    * Set the creation date of the note (stored in the mime header)
    */
  void setCreationDate( const KDateTime &creationDate );

  /**
    * Returns the creation date of the note
    */
  KDateTime creationDate() const;

  /**
    * Set the origin (creator) of the note (stored in the mime header)
    * This is usually the application creating the note.
    */
  void setFrom( const QString &from );

  /**
    * Returns the origin (creator) of the note
    */
  QString from() const;

  /**
    * Assemble a KMime message with the given values
    *
    * The message can then i.e. be stored inside an akonadi item
    */
  KMime::MessagePtr message() const;

private:
  //@cond PRIVATE
  class NoteMessageWrapperPrivate;
  NoteMessageWrapperPrivate * const d_ptr;
  Q_DECLARE_PRIVATE( NoteMessageWrapper )
  //@endcond
};

}
}

#endif
