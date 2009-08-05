/*
    This file is part of KAddressBook.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "displaynameeditwidget.h"

#include <QtCore/QString>
#include <QtGui/QActionGroup>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenu>

#include <kabc/addressee.h>
#include <kdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <ktoggleaction.h>

class DisplayNameEditWidget::LineEdit : public KLineEdit
{
  public:
    LineEdit( DisplayNameEditWidget *parent )
      : KLineEdit( parent ), mParent( parent )
    {
    }

  protected:
    // context menu handling
    virtual void contextMenuEvent( QContextMenuEvent *event )
    {
      mParent->contextMenuEvent( event );
    }

  private:
    DisplayNameEditWidget *mParent;
};

DisplayNameEditWidget::DisplayNameEditWidget( QWidget *parent )
  : QWidget( parent ),
    mDisplayType( FullName )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( KDialog::spacingHint() );

  mView = new LineEdit( this );
  layout->addWidget( mView );
}

DisplayNameEditWidget::~DisplayNameEditWidget()
{
}

void DisplayNameEditWidget::setReadOnly( bool readOnly )
{
  mView->setReadOnly( readOnly && (mDisplayType != CustomName) );
}

void DisplayNameEditWidget::setDisplayType( DisplayType type )
{
  mDisplayType = type;
  updateView();
}

DisplayNameEditWidget::DisplayType DisplayNameEditWidget::displayType() const
{
  return mDisplayType;
}

void DisplayNameEditWidget::loadContact( const KABC::Addressee &contact )
{
  mView->setText( contact.formattedName() );
  mContact = contact;

  updateView();
}

void DisplayNameEditWidget::storeContact( KABC::Addressee &contact ) const
{
  contact.setFormattedName( mView->text() );
}

void DisplayNameEditWidget::changeName( const KABC::Addressee &contact )
{
  const QString organization = mContact.organization();
  mContact = contact;
  mContact.setOrganization( organization );
  mContact.setFormattedName( mView->text() );

  updateView();
}

void DisplayNameEditWidget::changeOrganization( const QString &organization )
{
  mContact.setOrganization( organization );

  updateView();
}

void DisplayNameEditWidget::contextMenuEvent( QContextMenuEvent *event )
{
  QMenu menu;

  QActionGroup *group = new QActionGroup( this );

  KToggleAction *simpleNameAction = new KToggleAction( i18n( "Simple Name" ), group );
  KToggleAction *fullNameAction = new KToggleAction( i18n( "Full Name" ), group );
  KToggleAction *reverseNameWithCommaAction = new KToggleAction( i18n( "Reverse Name with Comma" ), group );
  KToggleAction *reverseNameAction = new KToggleAction( i18n( "Reverse Name" ), group );
  KToggleAction *organizationNameAction = new KToggleAction( i18n( "Organization Name" ), group );
  KToggleAction *customNameAction = new KToggleAction( i18n( "Custom" ), group );

  group->setExclusive( true );

  menu.addAction( simpleNameAction );
  menu.addAction( fullNameAction );
  menu.addAction( reverseNameWithCommaAction );
  menu.addAction( reverseNameAction );
  menu.addAction( organizationNameAction );
  menu.addAction( customNameAction );

  if ( mDisplayType == SimpleName )
    simpleNameAction->setChecked( true );
  if ( mDisplayType == FullName )
    fullNameAction->setChecked( true );
  if ( mDisplayType == ReverseNameWithComma )
    reverseNameWithCommaAction->setChecked( true );
  if ( mDisplayType == ReverseName )
    reverseNameAction->setChecked( true );
  if ( mDisplayType == Organization )
    organizationNameAction->setChecked( true );
  if ( mDisplayType == CustomName )
    customNameAction->setChecked( true );

  QAction *result = menu.exec( event->globalPos() );
  if ( result == simpleNameAction )
    mDisplayType = SimpleName;
  else if ( result == fullNameAction )
    mDisplayType = FullName;
  else if ( result == reverseNameWithCommaAction )
    mDisplayType = ReverseNameWithComma;
  else if ( result == reverseNameAction )
    mDisplayType = ReverseName;
  else if ( result == organizationNameAction )
    mDisplayType = Organization;
  else if ( result == customNameAction )
    mDisplayType = CustomName;

  delete group;

  updateView();
}

void DisplayNameEditWidget::updateView()
{
  QString text;

  switch ( mDisplayType ) {
    case SimpleName:
      text = mContact.givenName() + ' ' + mContact.familyName();
      break;
    case FullName:
      text = mContact.assembledName();
      break;
    case ReverseNameWithComma:
      text = mContact.familyName() + ", " + mContact.givenName();
      break;
    case ReverseName:
      text = mContact.familyName() + ' ' + mContact.givenName();
      break;
    case Organization:
      text = mContact.organization();
      break;
    case CustomName:
      text = mContact.formattedName();
    default:
      break;
  }

  mView->setText( text );
  mView->setReadOnly( mDisplayType != CustomName );
}

#include "displaynameeditwidget.moc"
