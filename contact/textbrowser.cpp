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

#include <kmime/kmime_util.h>

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>

using namespace Akonadi;

TextBrowser::TextBrowser( QWidget *parent )
  : KTextBrowser( parent )
{
}

void TextBrowser::slotCopyEmail()
{
#ifndef QT_NO_CLIPBOARD
  QClipboard* clip = QApplication::clipboard();
  // put the url into the mouse selection and the clipboard
  const QString address = KMime::decodeRFC2047String( KUrl(mLinkToCopy).path().toUtf8() );
  clip->setText( address, QClipboard::Clipboard );
  clip->setText( address, QClipboard::Selection );
#endif
}

#ifndef QT_NO_CONTEXTMENU
void TextBrowser::contextMenuEvent( QContextMenuEvent *event )
{
  QMenu *popup = createStandardContextMenu( event->pos() );
  if ( popup ) { // can be 0 on touch-only platforms
     QList<QAction*> actions = popup->actions();

     // inherited from KTextBrowser
     KIconTheme::assignIconsToContextMenu( KIconTheme::ReadOnlyText, actions );
#ifndef QT_NO_CLIPBOARD
     // hide the 'Copy Link Location' action if not an e-mail address
     mLinkToCopy = anchorAt( event->pos() );
     if ( mLinkToCopy.left( 7 ) != QLatin1String( "mailto:" ) ) {
       actions[ 1 ]->setVisible( false );
     } else {
       actions[ 1 ]->setText( i18n( "Copy e-mail address" ) );
       disconnect(actions[1]);
       connect(actions[1],SIGNAL(triggered(bool)),this,SLOT(slotCopyEmail()));
     }
#endif
     popup->exec( event->globalPos() );
     delete popup;
  }
}
#endif
