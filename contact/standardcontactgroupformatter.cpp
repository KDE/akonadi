/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "standardcontactgroupformatter.h"

#include <akonadi/contact/contactgroupexpandjob.h>
#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <kcolorscheme.h>
#include <kglobal.h>
#include <klocalizedstring.h>
#include <kstringhandler.h>

using namespace Akonadi;

StandardContactGroupFormatter::StandardContactGroupFormatter()
  : d( 0 )
{
}

StandardContactGroupFormatter::~StandardContactGroupFormatter()
{
}

QString StandardContactGroupFormatter::toHtml( HtmlForm form ) const
{
  KABC::ContactGroup group;
  const Akonadi::Item localItem = item();
  if ( localItem.isValid() && localItem.hasPayload<KABC::ContactGroup>() ) {
    group = localItem.payload<KABC::ContactGroup>();
  } else {
    group = contactGroup();
  }

  if ( group.name().isEmpty() && group.count() == 0 ) { // empty group
    return QString();
  }

  if ( group.contactReferenceCount() != 0 ) {
    // we got a contact group with unresolved references -> we have to resolve it ourself
    // this shouldn't be the normal case, actually the calling code should pass in an already resolved
    // contact group
    ContactGroupExpandJob *job = new ContactGroupExpandJob( group );
    if ( job->exec() ) {
      group.removeAllContactData();
      foreach ( const KABC::Addressee &contact, job->contacts() ) {
        group.append( KABC::ContactGroup::Data( contact.realName(), contact.preferredEmail() ) );
      }
    }
  }

  // Assemble all parts
  QString strGroup = QString::fromLatin1(
    "<table cellpadding=\"3\" cellspacing=\"0\" width=\"100%\">"
    "<tr>"
    "<td align=\"right\" valign=\"top\" width=\"30%\">"
    "<img src=\"%1\" width=\"100\" vspace=\"1\">" // image
    "</td>"
    "<td align=\"left\" width=\"70%\"><font size=\"+2\"><b>%2</b></font></td>" // name
    "</tr>"
    "</table>" )
      .arg( QLatin1String( "group_photo" ) )
      .arg( group.name() );

  strGroup += QLatin1String( "<table width=\"100%\">" );

  for ( uint i = 0; i < group.dataCount(); ++i ) {
    const KABC::ContactGroup::Data data = group.data( i );

    if ( data.email().isEmpty() ) {
      strGroup.append( QString::fromLatin1( "<tr><td align=\"right\" width=\"50%\"><b><font color=\"grey\">%1</font></b></td>"
                                            "<td width=\"50%\"></td></tr>" )
                     .arg( data.name() ) );
    } else {
      KABC::Addressee contact;
      contact.setFormattedName( data.name() );
      contact.insertEmail( data.email() );

      const QString fullEmail = QLatin1String( "<a href=\"mailto:" ) + QString::fromLatin1( KUrl::toPercentEncoding( contact.fullEmail() ) ) + QString::fromLatin1( "\">%1</a>" ).arg( contact.preferredEmail() );

      strGroup.append( QString::fromLatin1( "<tr><td align=\"right\" width=\"50%\"><b><font color=\"grey\">%1</font></b></td>"
                                            "<td valign=\"bottom\" align=\"left\" width=\"50%\"><font>&lt;%2&gt;</font></td></tr>" )
                     .arg( contact.realName() )
                     .arg( fullEmail ) );
    }
  }

  foreach ( const QVariantMap &map, additionalFields() ) {
    strGroup.append( QString::fromLatin1( "<tr><td colspan=\"2\">&nbsp;</td></tr><tr><td align=\"right\" width=\"30%\"><b><font color=\"grey\">%1</font></b></td>"
                                          "<td valign=\"bottom\" align=\"left\" width=\"50%\"><font>%2</font></td></tr>" )
                   .arg( map.value( QLatin1String( "title" ) ).toString() )
                   .arg( map.value( QLatin1String( "value" ) ).toString() ) );
  }

  strGroup.append( QString::fromLatin1( "</table>\n" ) );

  QString document = QString::fromLatin1( "<div align=\"center\">%1</div>" ).arg( strGroup );

  if ( form == EmbeddableForm ) {
    return document;
  }

  document = QString::fromLatin1(
    "<html>"
    "<head>"
    " <style type=\"text/css\">"
    "  a {text-decoration:none; color:%1}"
    " </style>"
    "</head>"
    "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
    "%3" // contact group part
    "</body>"
    "</html>" )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).foreground().color().name() )
     .arg( KColorScheme( QPalette::Active, KColorScheme::View ).background().color().name() )
     .arg( document );

  return document;
}
