/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTGROUPEDITOR_P_H
#define AKONADI_CONTACTGROUPEDITOR_P_H

#include "contactgroupeditor.h"

#include "ui_contactgroupeditor.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

class KJob;

namespace Akonadi
{

class ContactGroupModel;
class Monitor;

class ContactGroupEditor::Private
{
  public:
    Private( ContactGroupEditor *parent );
    ~Private();

    void itemFetchDone( KJob* );
    void parentCollectionFetchDone( KJob* );
    void storeDone( KJob* );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& );
    void memberChanged();
    void setReadOnly( bool );

    void adaptHeaderSizes();

    void loadContactGroup( const KABC::ContactGroup &group );
    bool storeContactGroup( KABC::ContactGroup &group );
    void setupMonitor();

    ContactGroupEditor *mParent;
    ContactGroupEditor::Mode mMode;
    Item mItem;
    Monitor *mMonitor;
    Collection mDefaultCollection;
    Ui::ContactGroupEditor mGui;
    bool mReadOnly;
    ContactGroupModel *mGroupModel;
};

}

#endif
