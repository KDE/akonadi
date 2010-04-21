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

#include "contactgroupviewer.h"

#include "contactgroupexpandjob.h"
#include "textbrowser_p.h"

#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kcolorscheme.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <kstringhandler.h>

#include <QtGui/QVBoxLayout>

using namespace Akonadi;

static QString contactsAsHtml( const QString &groupName, const KABC::Addressee::List &contacts );

class ContactGroupViewer::Private
{
  public:
    Private( ContactGroupViewer *parent )
      : mParent( parent ), mExpandJob( 0 )
    {
    }

    void slotMailClicked( const QString&, const QString &email )
    {
      QString name, address;

      // remove the 'mailto:' and split into name and email address
      KABC::Addressee::parseEmailAddress( email.mid( 7 ), name, address );

      emit mParent->emailClicked( name, address );
    }

    void _k_expandResult( KJob *job )
    {
      ContactGroupExpandJob *expandJob = qobject_cast<ContactGroupExpandJob*>( job );

      const KABC::Addressee::List contacts = expandJob->contacts();

      mBrowser->setHtml( contactsAsHtml( mGroupName, contacts ) );

      mExpandJob = 0;
    }

    ContactGroupViewer *mParent;
    TextBrowser *mBrowser;
    QString mGroupName;
    ContactGroupExpandJob *mExpandJob;
};

ContactGroupViewer::ContactGroupViewer( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  d->mBrowser = new TextBrowser;
  d->mBrowser->setNotifyClick( true );

  connect( d->mBrowser, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( slotMailClicked( const QString&, const QString& ) ) );

  layout->addWidget( d->mBrowser );

  // always fetch full payload for contacts
  fetchScope().fetchFullPayload();
}

ContactGroupViewer::~ContactGroupViewer()
{
  delete d;
}

Akonadi::Item ContactGroupViewer::contactGroup() const
{
  return ItemMonitor::item();
}

void ContactGroupViewer::setContactGroup( const Akonadi::Item &group )
{
  ItemMonitor::setItem( group );
}

void ContactGroupViewer::itemChanged( const Item &item )
{
  if ( !item.hasPayload<KABC::ContactGroup>() )
    return;

  static QPixmap groupPixmap = KIcon( QLatin1String( "x-mail-distribution-list" ) ).pixmap( QSize( 100, 140 ) );

  const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
  d->mGroupName = group.name();

  setWindowTitle( i18n( "Contact Group %1", group.name() ) );

  d->mBrowser->document()->addResource( QTextDocument::ImageResource,
                                        QUrl( QLatin1String( "group_photo" ) ),
                                        groupPixmap );

  if ( d->mExpandJob ) {
    disconnect( d->mExpandJob, SIGNAL( result( KJob* ) ), this, SLOT( _k_expandResult( KJob* ) ) );
    d->mExpandJob->kill();
  }

  d->mExpandJob = new ContactGroupExpandJob( group );
  connect( d->mExpandJob, SIGNAL( result( KJob* ) ), SLOT( _k_expandResult( KJob* ) ) );
  d->mExpandJob->start();
}

void ContactGroupViewer::itemRemoved()
{
  d->mBrowser->clear();
}

static QString contactsAsHtml( const QString &groupName, const KABC::Addressee::List &contacts )
{
  // Assemble all parts
  QString strGroup = QString::fromLatin1(
    "<table cellpadding=\"1\" cellspacing=\"0\" width=\"100%\">"
    "<tr>"
    "<td align=\"right\" valign=\"top\" width=\"30%\">"
    "<img src=\"%1\" width=\"75\" height=\"105\" vspace=\"1\">" // image
    "</td>"
    "<td align=\"left\" width=\"70%\"><font size=\"+2\"><b>%2</b></font></td>" // name
    "</tr>"
    "</table>" )
      .arg( QLatin1String( "group_photo" ) )
      .arg( groupName );

  strGroup += QLatin1String( "<table width=\"100%\">" );


  foreach ( const KABC::Addressee &contact, contacts ) {
    if ( contact.preferredEmail().isEmpty() ) {
      strGroup.append( QString::fromLatin1( "<tr><td align=\"right\" width=\"50%\"><b><font size=\"-1\" color=\"grey\">%1</font></b></td>"
                                            "<td width=\"50%\"></td></tr>" )
                     .arg( contact.realName() ) );
    } else {
      const QString fullEmail = QLatin1String( "<a href=\"mailto:" ) + QString::fromLatin1( KUrl::toPercentEncoding( contact.fullEmail() ) ) + QString::fromLatin1( "\">%1</a>" ).arg( contact.preferredEmail() );

      strGroup.append( QString::fromLatin1( "<tr><td align=\"right\" width=\"50%\"><b><font size=\"-1\" color=\"grey\">%1</font></b></td>"
                                            "<td valign=\"bottom\" align=\"left\" width=\"50%\"><font size=\"-1\">&lt;%2&gt;</font></td></tr>" )
                     .arg( contact.realName() )
                     .arg( fullEmail ) );
    }
  }

  strGroup.append( QString::fromLatin1( "</table>\n" ) );

  const QString document = QString::fromLatin1(
    "<html>"
    "<head>"
    " <style type=\"text/css\">"
    "  a {text-decoration:none; color:%1}"
    " </style>"
    "</head>"
    "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
    "%3" // contact part
    "</body>"
    "</html>" )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).foreground().color().name() )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).background().color().name() )
     .arg( strGroup );

  return document;
}

#include "contactgroupviewer.moc"
