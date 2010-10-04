/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef TEXTBROWSER_H
#define TEXTBROWSER_H

#include <kicontheme.h>
#include <ktextbrowser.h>

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>

namespace Akonadi {

/**
 * A convenience class to remove the 'Copy Link Location' action
 * from the context menu of KTextBrowser.
 */
class TextBrowser : public KTextBrowser
{
  public:
    TextBrowser( QWidget *parent = 0 )
      : KTextBrowser( parent )
    {
    }

  protected:
#ifndef QT_NO_CONTEXTMENU
    virtual void contextMenuEvent( QContextMenuEvent *event )
    {
      QMenu *popup = createStandardContextMenu( event->pos() );
      if ( popup ) { // can be 0 on touch-only platforms
        QList<QAction*> actions = popup->actions();

        // inherited from KTextBrowser
        KIconTheme::assignIconsToContextMenu( KIconTheme::ReadOnlyText, actions );

        // hide the 'Copy Link Location' action
        actions[ 1 ]->setVisible( false );

        popup->exec( event->globalPos() );
        delete popup;
      }
    }
#endif
};

}

#endif
