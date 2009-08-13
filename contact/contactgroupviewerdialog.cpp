/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contactgroupviewerdialog.h"

#include "contactgroupviewer.h"

#include <akonadi/item.h>
#include <klocale.h>

#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class ContactGroupViewerDialog::Private
{
  public:
    ContactGroupViewer *mViewer;
};

ContactGroupViewerDialog::ContactGroupViewerDialog( QWidget *parent )
  : KDialog( parent ), d( new Private )
{
  setCaption( i18n( "Show Contact Group" ) );
  setButtons( Ok );

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QVBoxLayout *layout = new QVBoxLayout( mainWidget );

  d->mViewer = new ContactGroupViewer;
  layout->addWidget( d->mViewer );

  setInitialSize( QSize( 500, 600 ) );
}

ContactGroupViewerDialog::~ContactGroupViewerDialog()
{
  delete d;
}

Akonadi::Item ContactGroupViewerDialog::contactGroup() const
{
  return d->mViewer->contactGroup();
}

void ContactGroupViewerDialog::setContactGroup( const Akonadi::Item &group )
{
  d->mViewer->setContactGroup( group );
}

#include "contactgroupviewerdialog.moc"
