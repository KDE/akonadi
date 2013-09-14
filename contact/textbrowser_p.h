/*
    This file is part of Akonadi Contact.
    Copyright (c) 2012 Montel Laurent <montel@kde.org>

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

#include <ktextbrowser.h>

namespace Akonadi {

/**
 * A convenience class to remove the 'Copy Link Location' action
 * from the context menu of KTextBrowser.
 */
class TextBrowser : public KTextBrowser
{
Q_OBJECT
public:
    explicit TextBrowser( QWidget *parent = 0 );
private Q_SLOTS:
    void slotCopyData();
protected:
#ifndef QT_NO_CONTEXTMENU
    virtual void contextMenuEvent( QContextMenuEvent *event );
#endif
private:
    QVariant mDataToCopy;
};

}

#endif
