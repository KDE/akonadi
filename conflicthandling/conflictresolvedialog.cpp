/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#include "conflictresolvedialog_p.h"

#include "abstractdifferencesreporter.h"
#include "differencesalgorithminterface.h"
#include "typepluginloader_p.h"

#include <QtGui/QHBoxLayout>

#include <kcolorscheme.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>

using namespace Akonadi;

static inline QString textToHTML( const QString &text )
{
  return Qt::convertFromPlainText( text );
}

class HtmlDifferencesReporter : public AbstractDifferencesReporter
{
  public:
    HtmlDifferencesReporter()
    {
    }

    QString toHtml() const
    {
      return header() + mContent + footer();
    }

    QString toHtml( const QString &name, const QString &leftValue, const QString &rightValue ) const
    {
      const QString content = QString::fromLatin1( "<tr><td align=\"right\"><b>%1:</b></td><td>%2</td><td></td><td>%3</td></tr>" )
                                                 .arg( name )
                                                 .arg( textToHTML( leftValue ) )
                                                 .arg( textToHTML( rightValue ) );
      return header() + content + footer();
    }

    void setPropertyNameTitle( const QString &title )
    {
      mNameTitle = title;
    }

    void setLeftPropertyValueTitle( const QString &title )
    {
      mLeftTitle = title;
    }

    void setRightPropertyValueTitle( const QString &title )
    {
      mRightTitle = title;
    }

    void addProperty( Mode mode, const QString &name, const QString &leftValue, const QString &rightValue )
    {
      switch ( mode ) {
        case NormalMode:
          mContent.append( QString::fromLatin1( "<tr><td align=\"right\"><b>%1:</b></td><td>%2</td><td></td><td>%3</td></tr>" )
                                              .arg( name )
                                              .arg( textToHTML( leftValue ) )
                                              .arg( textToHTML( rightValue ) ) );
         break;
        case ConflictMode:
          mContent.append( QString::fromLatin1( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#ff8686\">%2</td><td></td><td bgcolor=\"#ff8686\">%3</td></tr>" )
                                              .arg( name )
                                              .arg( textToHTML( leftValue ) )
                                              .arg( textToHTML( rightValue ) ) );
         break;
       case AdditionalLeftMode:
         mContent.append( QString::fromLatin1( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#9cff83\">%2</td><td></td><td></td></tr>" )
                                             .arg( name )
                                             .arg( textToHTML( leftValue ) ) );
         break;
       case AdditionalRightMode:
         mContent.append( QString::fromLatin1( "<tr><td align=\"right\"><b>%1:</b></td><td></td><td></td><td bgcolor=\"#9cff83\">%2</td></tr>" )
                                             .arg( name )
                                             .arg( textToHTML( rightValue ) ) );
         break;
      }
    }

  private:
    QString header() const
    {
      QString header = QLatin1String( "<html>" );
      header += QString::fromLatin1( "<body text=\"%1\" bgcolor=\"%2\">" )
                                   .arg( KColorScheme( QPalette::Active, KColorScheme::View ).foreground().color().name() )
                                   .arg( KColorScheme( QPalette::Active, KColorScheme::View ).background().color().name() );
      header += QLatin1String( "<center><table>" );
      header += QString::fromLatin1( "<tr><th align=\"center\">%1</th><th align=\"center\">%2</th><td>&nbsp;</td><th align=\"center\">%3</th></tr>" )
                                   .arg( mNameTitle )
                                   .arg( mLeftTitle )
                                   .arg( mRightTitle );

      return header;
    }

    QString footer() const
    {
      return QLatin1String( "</table></center>"
                            "</body>"
                            "</html>" );
    }

    QString mContent;
    QString mNameTitle;
    QString mLeftTitle;
    QString mRightTitle;
};


ConflictResolveDialog::ConflictResolveDialog( QWidget *parent )
  : KDialog( parent ), mResolveStrategy( ConflictHandler::UseBothItems )
{
  setButtons( User1 | User2 | User3 );
  setDefaultButton( User3 );

  button( User3 )->setText( i18n( "Take left one" ) );
  button( User2 )->setText( i18n( "Take right one" ) );
  button( User1 )->setText( i18n( "Keep both" ) );

  connect( this, SIGNAL( user1Clicked() ), SLOT( slotUseBothItemsChoosen() ) );
  connect( this, SIGNAL( user2Clicked() ), SLOT( slotUseOtherItemChoosen() ) );
  connect( this, SIGNAL( user3Clicked() ), SLOT( slotUseLocalItemChoosen() ) );

  QWidget *mainWidget = new QWidget;
  QHBoxLayout *layout = new QHBoxLayout( mainWidget );

  mView = new KTextBrowser;

  layout->addWidget( mView );

  setMainWidget( mainWidget );
}

void ConflictResolveDialog::setConflictingItems( const Akonadi::Item &localItem, const Akonadi::Item &otherItem )
{
  mLocalItem = localItem;
  mOtherItem = otherItem;

  if ( mLocalItem.hasPayload() && mOtherItem.hasPayload() ) {
    HtmlDifferencesReporter reporter;

    QObject *object = TypePluginLoader::objectForMimeType( localItem.mimeType() );
    if ( object ) {
      DifferencesAlgorithmInterface *algorithm = qobject_cast<DifferencesAlgorithmInterface*>( object );
      if ( algorithm ) {
        algorithm->compare( &reporter, localItem, otherItem );
        mView->setHtml( reporter.toHtml() );
        return;
      }
    }

    const QString html = reporter.toHtml( i18n( "Data" ),
                                          QString::fromUtf8( mLocalItem.payloadData() ),
                                          QString::fromUtf8( mOtherItem.payloadData() ) );
    mView->setHtml( html );
  } else {
    mView->setHtml( QLatin1String( "<html><body>Conflicting flags or attributes</body></html>" ) );
  }
}

ConflictHandler::ResolveStrategy ConflictResolveDialog::resolveStrategy() const
{
  return mResolveStrategy;
}

void ConflictResolveDialog::slotUseLocalItemChoosen()
{
  mResolveStrategy = ConflictHandler::UseLocalItem;
  accept();
}

void ConflictResolveDialog::slotUseOtherItemChoosen()
{
  mResolveStrategy = ConflictHandler::UseOtherItem;
  accept();
}

void ConflictResolveDialog::slotUseBothItemsChoosen()
{
  mResolveStrategy = ConflictHandler::UseBothItems;
  accept();
}

#include "conflictresolvedialog_p.moc"
