/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

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

#include "textbrowser_p.h"
#include <kicontheme.h>
#include <ktextbrowser.h>
#include <klocalizedstring.h>
#include <KUrl>
#include <KAction>
#include <KStandardAction>

#include <kmime/kmime_util.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include <QTextBlock>

using namespace Akonadi;

TextBrowser::TextBrowser( QWidget *parent )
  : KTextBrowser( parent )
{
}


void TextBrowser::slotCopyData()
{
#ifndef QT_NO_CLIPBOARD
  QClipboard *clip = QApplication::clipboard();
  // put the data into the mouse selection and the clipboard
  if ( mDataToCopy.type() == QVariant::Pixmap ) {
    clip->setPixmap( mDataToCopy.value<QPixmap>(), QClipboard::Clipboard );
    clip->setPixmap( mDataToCopy.value<QPixmap>(), QClipboard::Selection );
  } else {
    clip->setText( mDataToCopy.toString(), QClipboard::Clipboard );
    clip->setText( mDataToCopy.toString(), QClipboard::Selection );
  }
#endif
}


#ifndef QT_NO_CONTEXTMENU
void TextBrowser::contextMenuEvent( QContextMenuEvent *event )
{
#ifndef QT_NO_CLIPBOARD
  QMenu popup;

  KAction *act = KStandardAction::copy( this, SLOT(copy()), this );
  act->setEnabled( !textCursor().selectedText().isEmpty() );
  act->setShortcut( QKeySequence() );
  popup.addAction( act );

  // Create a new action to correspond with what is under the click
  act = new KAction( i18nc( "@action:inmenu Copy the text of a general item", "Copy Item" ), this );

  mDataToCopy.clear();					// nothing found to copy yet

  QString link = anchorAt( event->pos() );
  if ( !link.isEmpty() ) {
    if ( link.startsWith( QLatin1String( "mailto:" ) ) ) {
      mDataToCopy = KMime::decodeRFC2047String( KUrl( link ).path().toUtf8() );
      // Action text matches that used in KMail
      act->setText( i18nc( "@action:inmenu Copy a displayed email address", "Copy Email Address" ) );
    } else {
      // A link, but it could be one of our internal ones.  There is
      // no point in copying these.  Internal links are always in the
      // form "protocol:?argument", whereas valid external links should
      // be in the form starting with "protocol://".
      if ( !link.contains( QRegExp( QLatin1String( "^\\w+:\\?" ) ) ) ) {
        mDataToCopy = link;
        // Action text matches that used in Konqueror
        act->setText( i18nc( "@action:inmenu Copy a link URL", "Copy Link URL" ) );
      }
    }
  }

  if ( !mDataToCopy.isValid() ) {			// no link was found above
    QTextCursor curs = cursorForPosition( event->pos() );
    QString text = curs.block().text();			// try the text under cursor

    if ( !text.isEmpty() ) {

      // curs().block().text() over an image (contact photo or QR code)
      // returns a string starting with the character 0xFFFC (Unicode
      // object replacement character).  See the documentation for
      // QTextImageFormat.
      if ( text.startsWith( QChar( 0xFFFC ) ) ) {
        QTextCharFormat charFormat = curs.charFormat();
        if ( charFormat.isImageFormat() ) {
          QTextImageFormat imageFormat = charFormat.toImageFormat();
          QString imageName = imageFormat.name();
          QVariant imageResource = document()->resource( QTextDocument::ImageResource,
                                                         QUrl( imageName ) );

          QPixmap pix = imageResource.value<QPixmap>();
          if ( !pix.isNull() ) {

            // There may be other images (e.g. contact type icons) that
            // there is no point in copying.
            if ( imageName == QLatin1String( "contact_photo" ) ) {
              mDataToCopy = pix;
              act->setText( i18nc( "@action:inmenu Copy a contact photo", "Copy Photo" ) );
            } else if ( imageName == QLatin1String( "datamatrix" ) ||
                        imageName == QLatin1String( "qrcode" ) ) {
              mDataToCopy = pix;
              act->setText( i18nc( "@action:inmenu Copy a QR or Datamatrix image", "Copy Code" ) );
            }
          }
        }
      } else {

        // Added by our formatter (but not I18N'ed) for a mobile
        // telephone number.  See
        // kdepim/kaddressbook/grantlee/grantleecontactformatter.cpp and
        // kdepimlibs/akonadi/contact/standardcontactformatter.cpp
        text.remove( QRegExp( QLatin1String( "\\s*\\(SMS\\)$" ) ) );

        // For an item which was formatted with line breaks (as <br>
        // in HTML), the returned text contains the character 0x2028
        // (Unicode line separator).  Convert any of these back to newlines.
        text.replace( QChar( 0x2028 ), QLatin1Char( '\n' ) );

        mDataToCopy = text;
      }
    }
  }

  if ( mDataToCopy.isValid() ) {
    connect( act, SIGNAL(triggered(bool)), SLOT(slotCopyData()) );
  } else {
    act->setEnabled( false );
  }

  popup.addAction( act );
  popup.exec( event->globalPos() );
#endif
}
#endif
