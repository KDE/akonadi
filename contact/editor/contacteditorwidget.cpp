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

#include "contacteditorwidget.h"

#include "addresseditwidget.h"
#include "categorieseditwidget.h"
#include "contacteditorpageplugin.h"
#include "contactmetadata_p.h"
#include "customfieldseditwidget.h"
#include "dateeditwidget.h"
#include "displaynameeditwidget.h"
#include "emaileditwidget.h"
#include "freebusyeditwidget.h"
#include "geoeditwidget.h"
#include "imagewidget.h"
#include "imeditwidget.h"
#include "nameeditwidget.h"
#include "phoneeditwidget.h"
#include "soundeditwidget.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klineedit.h>
#include <klocalizedstring.h>
#include <kstandarddirs.h>
#include <ktabwidget.h>
#include <ktextedit.h>

#include <QtCore/QDirIterator>
#include <QtCore/QPluginLoader>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>

class ContactEditorWidget::Private
{
  public:
    Private( ContactEditorWidget::DisplayMode displayMode, ContactEditorWidget *parent )
      : mDisplayMode(displayMode), mParent( parent ), mCustomFieldsWidget(0)
    {
    }

    void initGui();
    void initGuiContactTab();
    void initGuiLocationTab();
    void initGuiBusinessTab();
    void initGuiPersonalTab();
    void initGuiNotesTab();
    void initGuiCustomFieldsTab();

    void loadCustomPages();

    QString loadCustom( const KABC::Addressee &contact, const QString &key ) const;
    void storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const;

    ContactEditorWidget::DisplayMode mDisplayMode;
    ContactEditorWidget *mParent;
    KTabWidget *mTabWidget;

    // widgets from name group
    NameEditWidget *mNameWidget;
    ImageWidget *mPhotoWidget;
    DisplayNameEditWidget *mDisplayNameWidget;
    KLineEdit *mNickNameWidget;
    SoundEditWidget *mPronunciationWidget;

    // widgets from Internet group
    EmailEditWidget *mEmailWidget;
    KLineEdit *mHomepageWidget;
    KLineEdit *mBlogWidget;
    IMEditWidget *mIMWidget;

    // widgets from phones group
    PhoneEditWidget *mPhonesWidget;

    CategoriesEditWidget *mCategoriesWidget;

    KComboBox* mMailPreferFormatting;
    QCheckBox *mAllowRemoteContent;

    // widgets from addresses group
    AddressEditWidget *mAddressesWidget;

    // widgets from coordinates group
    GeoEditWidget *mCoordinatesWidget;

    // widgets from general group
    ImageWidget *mLogoWidget;
    KLineEdit *mOrganizationWidget;
    KLineEdit *mProfessionWidget;
    KLineEdit *mTitleWidget;
    KLineEdit *mDepartmentWidget;
    KLineEdit *mOfficeWidget;
    KLineEdit *mManagerWidget;
    KLineEdit *mAssistantWidget;

    // widgets from groupware group
    FreeBusyEditWidget *mFreeBusyWidget;

    // widgets from notes group
    KTextEdit *mNotesWidget;

    // widgets from dates group
    DateEditWidget *mBirthdateWidget;
    DateEditWidget *mAnniversaryWidget;

    // widgets from family group
    KLineEdit *mPartnerWidget;

    // widgets from custom fields group
    CustomFieldsEditWidget *mCustomFieldsWidget;

    // custom editor pages
    QList<Akonadi::ContactEditorPagePlugin*> mCustomPages;
};

void ContactEditorWidget::Private::initGui()
{
  QVBoxLayout *layout = new QVBoxLayout( mParent );
  layout->setMargin( 0 );

  mTabWidget = new KTabWidget( mParent );
  layout->addWidget( mTabWidget );

  initGuiContactTab();
  initGuiLocationTab();
  initGuiBusinessTab();
  initGuiPersonalTab();
  initGuiNotesTab();
  if (mDisplayMode == FullMode) {
    initGuiCustomFieldsTab();
    loadCustomPages();
  }
}

void ContactEditorWidget::Private::initGuiContactTab()
{
  QWidget *widget = new QWidget;
  QGridLayout *layout = new QGridLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab", "Contact" ) );

  QGroupBox *nameGroupBox = new QGroupBox( i18nc( "@title:group Name related properties of a contact", "Name" ) );
  QGroupBox *internetGroupBox = new QGroupBox( i18nc( "@title:group", "Internet" ) );
  QGroupBox *phonesGroupBox = new QGroupBox( i18nc( "@title:group", "Phones" ) );

  nameGroupBox->setMinimumSize(320,200);
  layout->addWidget( nameGroupBox, 0, 0 );
  layout->addWidget( internetGroupBox, 0, 1 );
  layout->addWidget( phonesGroupBox, 1, 0, 4, 1 );

  QGridLayout *nameLayout = new QGridLayout( nameGroupBox );
  QGridLayout *internetLayout = new QGridLayout( internetGroupBox );
  QGridLayout *phonesLayout = new QGridLayout( phonesGroupBox );

  QLabel *label = 0;

  // setup name group box
  label = new QLabel( i18nc( "@label The name of a contact", "Name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 0, 0 );

  mNameWidget = new NameEditWidget;
  label->setBuddy( mNameWidget );
  nameLayout->addWidget( mNameWidget, 0, 1 );

  mPhotoWidget = new ImageWidget( ImageWidget::Photo );
  nameLayout->addWidget( mPhotoWidget, 0, 2, 4, 1 );

  label = new QLabel( i18nc( "@label The display name of a contact", "Display:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 1, 0 );

  mDisplayNameWidget = new DisplayNameEditWidget;
  label->setBuddy( mDisplayNameWidget );
  nameLayout->addWidget( mDisplayNameWidget, 1, 1 );

  label = new QLabel( i18nc( "@label The nickname of a contact", "Nickname:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 2, 0 );

  mNickNameWidget = new KLineEdit;
  mNickNameWidget->setTrapReturnKey(true);
  label->setBuddy( mNickNameWidget );
  nameLayout->addWidget( mNickNameWidget, 2, 1 );

  label = new QLabel( i18nc( "@label The pronunciation of a contact's name", "Pronunciation:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 3, 0 );

  mPronunciationWidget = new SoundEditWidget;
  label->setBuddy( mPronunciationWidget );
  nameLayout->addWidget( mPronunciationWidget, 3, 1 );

  nameLayout->setRowStretch( 4, 1 );

  // setup Internet group box
  label = new QLabel( i18nc( "@label The email address of a contact", "Email:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 0, 0 );

  mEmailWidget = new EmailEditWidget;
  label->setBuddy( mEmailWidget );
  internetLayout->addWidget( mEmailWidget, 0, 1 );

  label = new QLabel( i18nc( "@label The homepage URL of a contact", "Homepage:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 1, 0 );

  mHomepageWidget = new KLineEdit;
  mHomepageWidget->setTrapReturnKey(true);
  label->setBuddy( mHomepageWidget );
  internetLayout->addWidget( mHomepageWidget, 1, 1 );

  label = new QLabel( i18nc( "@label The blog URL of a contact", "Blog:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 2, 0 );

  mBlogWidget = new KLineEdit;
  mBlogWidget->setTrapReturnKey(true);
  label->setBuddy( mBlogWidget );
  internetLayout->addWidget( mBlogWidget, 2, 1 );

  label = new QLabel( i18nc( "@label The instant messaging address of a contact", "Messaging:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 3, 0 );

  mIMWidget = new IMEditWidget;
  label->setBuddy( mIMWidget );
  internetLayout->addWidget( mIMWidget, 3, 1 );

  internetLayout->setRowStretch( 4, 1 );

  // setup phones group box
  mPhonesWidget = new PhoneEditWidget;
  phonesLayout->addWidget( mPhonesWidget, 0, 0 );

  //phonesLayout->setRowStretch( 1, 1 );

  // setup categories section
  QHBoxLayout *categoriesLayout = new QHBoxLayout;
  label = new QLabel( i18nc( "@label The categories of a contact", "Categories:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );

  mCategoriesWidget = new CategoriesEditWidget;
  label->setBuddy( mCategoriesWidget );

  categoriesLayout->addWidget( label );
  categoriesLayout->addWidget( mCategoriesWidget );

  layout->addLayout( categoriesLayout, 1, 1 );

  QGroupBox *receivedMessageGroupBox = new QGroupBox( i18n("Messages") );
  layout->addWidget( receivedMessageGroupBox, 2, 1 );

  QVBoxLayout *vbox = new QVBoxLayout(receivedMessageGroupBox);

  QHBoxLayout *mailPreferFormattingLayout = new QHBoxLayout;
  label = new QLabel( i18n( "Show messages received from this contact as:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  mMailPreferFormatting = new KComboBox;
  label->setBuddy( mMailPreferFormatting );
  QStringList listFormat;
  listFormat << i18n( "Default" ) << i18n( "Plain Text" ) << i18n( "HTML" );
  mMailPreferFormatting->addItems( listFormat );
  mailPreferFormattingLayout->addWidget( label );
  mailPreferFormattingLayout->addWidget( mMailPreferFormatting );


  vbox->addLayout( mailPreferFormattingLayout );

  mAllowRemoteContent = new QCheckBox( i18n( "Allow remote content in received HTML messages" ) );
  vbox->addWidget( mAllowRemoteContent );

  layout->setRowStretch( 4,1 );
}

void ContactEditorWidget::Private::initGuiLocationTab()
{
  QWidget *widget = new QWidget;
  QHBoxLayout *layout = new QHBoxLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab", "Location" ) );

  QGroupBox *addressesGroupBox = new QGroupBox( i18nc( "@title:group", "Addresses" ) );
  QGroupBox *coordinatesGroupBox = new QGroupBox( i18nc( "@title:group", "Coordinates" ) );

  layout->addWidget( addressesGroupBox );
  layout->addWidget( coordinatesGroupBox );

  QGridLayout *addressesLayout = new QGridLayout( addressesGroupBox );
  QGridLayout *coordinatesLayout = new QGridLayout( coordinatesGroupBox );

  // setup addresses group box
  mAddressesWidget = new AddressEditWidget( addressesGroupBox );
  mAddressesWidget->setMinimumHeight( 200 );
  addressesLayout->addWidget( mAddressesWidget, 0, 0 );
  addressesLayout->setRowStretch( 1, 1 );

  // setup coordinates group box
  mCoordinatesWidget = new GeoEditWidget;
  coordinatesLayout->addWidget( mCoordinatesWidget, 0, 0 );
  coordinatesLayout->setRowStretch( 1, 1 );
}

void ContactEditorWidget::Private::initGuiBusinessTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab", "Business" ) );

  QGroupBox *generalGroupBox = new QGroupBox( i18nc( "@title:group General properties of a contact", "General" ) );
  QGroupBox *groupwareGroupBox = new QGroupBox( i18nc( "@title:group", "Groupware" ) );

  layout->addWidget( generalGroupBox );
  layout->addWidget( groupwareGroupBox );

  QGridLayout *generalLayout = new QGridLayout( generalGroupBox );
  QGridLayout *groupwareLayout = new QGridLayout( groupwareGroupBox );

  QLabel *label = 0;

  // setup general group box
  mLogoWidget = new ImageWidget( ImageWidget::Logo );
  generalLayout->addWidget( mLogoWidget, 0, 2, 6, 1, Qt::AlignTop );

  label = new QLabel( i18nc( "@label The organization of a contact", "Organization:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 0, 0 );

  mOrganizationWidget = new KLineEdit;
  mOrganizationWidget->setTrapReturnKey(true);
  label->setBuddy( mOrganizationWidget );
  generalLayout->addWidget( mOrganizationWidget, 0, 1 );

  label = new QLabel( i18nc( "@label The profession of a contact", "Profession:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 1, 0 );

  mProfessionWidget = new KLineEdit;
  mProfessionWidget->setTrapReturnKey(true);
  label->setBuddy( mProfessionWidget );
  generalLayout->addWidget( mProfessionWidget, 1, 1 );

  label = new QLabel( i18nc( "@label The title of a contact", "Title:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 2, 0 );

  mTitleWidget = new KLineEdit;
  mTitleWidget->setTrapReturnKey(true);
  label->setBuddy( mTitleWidget );
  generalLayout->addWidget( mTitleWidget , 2, 1 );

  label = new QLabel( i18nc( "@label The department of a contact", "Department:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 3, 0 );

  mDepartmentWidget = new KLineEdit;
  mDepartmentWidget->setTrapReturnKey(true);
  label->setBuddy( mDepartmentWidget );
  generalLayout->addWidget( mDepartmentWidget, 3, 1 );

  label = new QLabel( i18nc( "@label The office of a contact", "Office:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 4, 0 );

  mOfficeWidget = new KLineEdit;
  mOfficeWidget->setTrapReturnKey(true);
  label->setBuddy( mOfficeWidget );
  generalLayout->addWidget( mOfficeWidget, 4, 1 );

  label = new QLabel( i18nc( "@label The manager's name of a contact", "Manager's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 5, 0 );

  mManagerWidget = new KLineEdit;
  mManagerWidget->setTrapReturnKey(true);
  label->setBuddy( mManagerWidget );
  generalLayout->addWidget( mManagerWidget, 5, 1 );

  label = new QLabel( i18nc( "@label The assistant's name of a contact", "Assistant's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 6, 0 );

  mAssistantWidget = new KLineEdit;
  mAssistantWidget->setTrapReturnKey(true);
  label->setBuddy( mAssistantWidget );
  generalLayout->addWidget( mAssistantWidget, 6, 1 );

  // setup groupware group box
  label = new QLabel( i18nc( "@label The free/busy information of a contact", "Free/Busy:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  groupwareLayout->addWidget( label, 0, 0 );

  mFreeBusyWidget = new FreeBusyEditWidget;
  label->setBuddy( mFreeBusyWidget );
  groupwareLayout->addWidget( mFreeBusyWidget, 0, 1 );
  groupwareLayout->setRowStretch( 1, 1 );
}

void ContactEditorWidget::Private::initGuiPersonalTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab Personal properties of a contact", "Personal" ) );

  QGroupBox *datesGroupBox = new QGroupBox( i18nc( "@title:group Date related properties of a contact", "Dates" ) );
  QGroupBox *familyGroupBox = new QGroupBox( i18nc( "@title:group Family related properties of a contact", "Family" ) );

  layout->addWidget( datesGroupBox );
  layout->addWidget( familyGroupBox );

  QGridLayout *datesLayout = new QGridLayout( datesGroupBox );
  QGridLayout *familyLayout = new QGridLayout( familyGroupBox );

  QLabel *label = 0;

  // setup dates group box
  label = new QLabel( i18nc( "@label The birthdate of a contact", "Birthdate:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 0, 0 );

  mBirthdateWidget = new DateEditWidget( DateEditWidget::Birthday );
  label->setBuddy( mBirthdateWidget );
  datesLayout->addWidget( mBirthdateWidget, 0, 1 );

  label = new QLabel( i18nc( "@label The wedding anniversary of a contact", "Anniversary:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 1, 0 );

  mAnniversaryWidget = new DateEditWidget( DateEditWidget::Anniversary );
  label->setBuddy( mAnniversaryWidget );
  datesLayout->addWidget( mAnniversaryWidget, 1, 1 );

  datesLayout->setRowStretch( 2, 1 );
  datesLayout->setColumnStretch( 1, 1 );

  // widgets from family group
  label = new QLabel( i18nc( "@label The partner's name of a contact", "Partner's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  familyLayout->addWidget( label, 0, 0 );

  mPartnerWidget = new KLineEdit;
  mPartnerWidget->setTrapReturnKey(true);
  label->setBuddy( mPartnerWidget );
  familyLayout->addWidget( mPartnerWidget, 0, 1 );

  familyLayout->setRowStretch( 1, 1 );
}

void ContactEditorWidget::Private::initGuiNotesTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab", "Notes" ) );

  mNotesWidget = new KTextEdit;
  mNotesWidget->setAcceptRichText(false);
  layout->addWidget( mNotesWidget );
}

void ContactEditorWidget::Private::initGuiCustomFieldsTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18nc( "@title:tab", "Custom Fields" ) );

  mCustomFieldsWidget = new CustomFieldsEditWidget;
  layout->addWidget( mCustomFieldsWidget );
}

void ContactEditorWidget::Private::loadCustomPages()
{
  qDeleteAll( mCustomPages );
  mCustomPages.clear();

  const QString pluginDirectory = KStandardDirs::locate( "lib", QLatin1String( "akonadi/contact/editorpageplugins/" ) );
  QDirIterator it( pluginDirectory, QDir::Files );
  while ( it.hasNext() ) {
    QPluginLoader loader( it.next() );
    if ( !loader.load() ) {
      continue;
    }

    Akonadi::ContactEditorPagePlugin *plugin = qobject_cast<Akonadi::ContactEditorPagePlugin*>( loader.instance() );
    if ( !plugin ) {
      continue;
    }

    mCustomPages.append( plugin );
  }

  foreach ( Akonadi::ContactEditorPagePlugin *plugin, mCustomPages ) {
    mTabWidget->addTab( plugin, plugin->title() );
  }
}

QString ContactEditorWidget::Private::loadCustom( const KABC::Addressee &contact, const QString &key ) const
{
  return contact.custom( QLatin1String( "KADDRESSBOOK" ), key );
}

void ContactEditorWidget::Private::storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const
{
  if ( value.isEmpty() ) {
    contact.removeCustom( QLatin1String( "KADDRESSBOOK" ), key );
  } else {
    contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), key, value );
  }
}

ContactEditorWidget::ContactEditorWidget( QWidget* )
  : d( new Private( FullMode, this ) )
{
  d->initGui();

  connect( d->mNameWidget, SIGNAL(nameChanged(KABC::Addressee)),
           d->mDisplayNameWidget, SLOT(changeName(KABC::Addressee)) );
  connect( d->mOrganizationWidget, SIGNAL(textChanged(QString)),
           d->mDisplayNameWidget, SLOT(changeOrganization(QString)) );
}

ContactEditorWidget::ContactEditorWidget( ContactEditorWidget::DisplayMode displayMode, QWidget * )
  : d( new Private( displayMode, this ) )
{
  d->initGui();

  connect( d->mNameWidget, SIGNAL(nameChanged(KABC::Addressee)),
           d->mDisplayNameWidget, SLOT(changeName(KABC::Addressee)) );
  connect( d->mOrganizationWidget, SIGNAL(textChanged(QString)),
           d->mDisplayNameWidget, SLOT(changeOrganization(QString)) );
}

ContactEditorWidget::~ContactEditorWidget()
{
  delete d;
}

void ContactEditorWidget::loadContact( const KABC::Addressee &contact, const Akonadi::ContactMetaData &metaData )
{
  // name group
  d->mPhotoWidget->loadContact( contact );
  d->mNameWidget->loadContact( contact );
  d->mDisplayNameWidget->loadContact( contact );
  d->mNickNameWidget->setText( contact.nickName() );
  d->mPronunciationWidget->loadContact( contact );

  // Internet group
  d->mEmailWidget->loadContact( contact );
  d->mHomepageWidget->setUrl( contact.url() );
  d->mBlogWidget->setText( d->loadCustom( contact, QLatin1String( "BlogFeed" ) ) );
  d->mIMWidget->loadContact( contact );

  // phones group
  d->mPhonesWidget->loadContact( contact );

  // categories section
  d->mCategoriesWidget->loadContact( contact );

  const QString mailPreferedFormatting = d->loadCustom( contact, QLatin1String( "MailPreferedFormatting" ) );
  if ( mailPreferedFormatting.isEmpty() ) {
    d->mMailPreferFormatting->setCurrentIndex( 0 );
  } else if ( mailPreferedFormatting == QLatin1String( "TEXT" ) ) {
    d->mMailPreferFormatting->setCurrentIndex( 1 );
  } else if ( mailPreferedFormatting == QLatin1String( "HTML" ) ) {
    d->mMailPreferFormatting->setCurrentIndex( 2 );
  } else {
    d->mMailPreferFormatting->setCurrentIndex( 0 );
  }

  const QString mailAllowToRemoteContent = d->loadCustom( contact, QLatin1String( "MailAllowToRemoteContent" ) );
  d->mAllowRemoteContent->setChecked( mailAllowToRemoteContent == QLatin1String( "TRUE" ) );

  // address group
  d->mAddressesWidget->loadContact( contact );

  // coordinates group
  d->mCoordinatesWidget->loadContact( contact );

  // general group
  d->mLogoWidget->loadContact( contact );
  d->mOrganizationWidget->setText( contact.organization() );
  d->mProfessionWidget->setText( d->loadCustom( contact, QLatin1String( "X-Profession" ) ) );
  d->mTitleWidget->setText( contact.title() );
  d->mDepartmentWidget->setText( contact.department() );
  d->mOfficeWidget->setText( d->loadCustom( contact, QLatin1String( "X-Office" ) ) );
  d->mManagerWidget->setText( d->loadCustom( contact, QLatin1String( "X-ManagersName" ) ) );
  d->mAssistantWidget->setText( d->loadCustom( contact, QLatin1String( "X-AssistantsName" ) ) );

  // groupware group
  d->mFreeBusyWidget->loadContact( contact );

  // notes group
  d->mNotesWidget->setPlainText( contact.note() );

  // dates group
  d->mBirthdateWidget->setDate( contact.birthday().date() );
  d->mAnniversaryWidget->setDate( QDate::fromString( d->loadCustom( contact, QLatin1String( "X-Anniversary" ) ),
                                                     Qt::ISODate ) );

  // family group
  d->mPartnerWidget->setText( d->loadCustom( contact, QLatin1String( "X-SpousesName" ) ) );

  d->mDisplayNameWidget->setDisplayType( (DisplayNameEditWidget::DisplayType)metaData.displayNameMode() );

  if (d->mDisplayMode == FullMode) {
    // custom fields group
    d->mCustomFieldsWidget->setLocalCustomFieldDescriptions( metaData.customFieldDescriptions() );
    d->mCustomFieldsWidget->loadContact( contact );

    // custom pages
    foreach ( Akonadi::ContactEditorPagePlugin *plugin, d->mCustomPages ) {
      plugin->loadContact( contact );
    }
  }
}

void ContactEditorWidget::storeContact( KABC::Addressee &contact, Akonadi::ContactMetaData &metaData ) const
{
  // name group
  d->mPhotoWidget->storeContact( contact );
  d->mNameWidget->storeContact( contact );
  d->mDisplayNameWidget->storeContact( contact );
  contact.setNickName( d->mNickNameWidget->text().trimmed() );
  d->mPronunciationWidget->storeContact( contact );

  // Internet group
  d->mEmailWidget->storeContact( contact );
  contact.setUrl( KUrl( d->mHomepageWidget->text().trimmed() ) );
  d->storeCustom( contact, QLatin1String( "BlogFeed" ), d->mBlogWidget->text().trimmed() );
  d->mIMWidget->storeContact( contact );

  // phones group
  d->mPhonesWidget->storeContact( contact );

  // categories section
  d->mCategoriesWidget->storeContact( contact );

  QString mailPreferedFormatting;
  const int index = d->mMailPreferFormatting->currentIndex();
  if ( index == 0 ) {
    //Nothing => remove custom variable
  } else if ( index == 1 ) {
    mailPreferedFormatting = QLatin1String( "TEXT" );
  } else if ( index == 2 ) {
    mailPreferedFormatting = QLatin1String( "HTML" );
  }
  d->storeCustom( contact, QLatin1String( "MailPreferedFormatting" ), mailPreferedFormatting );

  QString mailAllowToRemoteContent;
  if ( d->mAllowRemoteContent->isChecked() ) {
    mailAllowToRemoteContent = QLatin1String( "TRUE" );
  }
  d->storeCustom( contact, QLatin1String( "MailAllowToRemoteContent" ), mailAllowToRemoteContent );

  // address group
  d->mAddressesWidget->storeContact( contact );

  // coordinates group
  d->mCoordinatesWidget->storeContact( contact );

  // general group
  d->mLogoWidget->storeContact( contact );
  contact.setOrganization( d->mOrganizationWidget->text() );
  d->storeCustom( contact, QLatin1String( "X-Profession" ), d->mProfessionWidget->text().trimmed() );
  contact.setTitle( d->mTitleWidget->text().trimmed() );
  contact.setDepartment( d->mDepartmentWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-Office" ), d->mOfficeWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-ManagersName" ), d->mManagerWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-AssistantsName" ), d->mAssistantWidget->text().trimmed() );

  // groupware group
  d->mFreeBusyWidget->storeContact( contact );

  // notes group
  contact.setNote( d->mNotesWidget->toPlainText() );

  // dates group
  QDateTime birthday = QDateTime( d->mBirthdateWidget->date(), QTime(), contact.birthday().timeSpec() );
  // This is needed because the constructor above sets the time component
  // of the QDateTime to midnight.  We want it to stay invalid.
  birthday.setTime( QTime() );

  contact.setBirthday( birthday );
  d->storeCustom( contact, QLatin1String( "X-Anniversary" ), d->mAnniversaryWidget->date().toString( Qt::ISODate ) );

  // family group
  d->storeCustom( contact, QLatin1String( "X-SpousesName" ), d->mPartnerWidget->text().trimmed() );

  if (d->mDisplayMode == FullMode) {
    // custom fields group
    d->mCustomFieldsWidget->storeContact( contact );
    metaData.setCustomFieldDescriptions( d->mCustomFieldsWidget->localCustomFieldDescriptions() );

    metaData.setDisplayNameMode( d->mDisplayNameWidget->displayType() );

    // custom pages
    foreach ( Akonadi::ContactEditorPagePlugin *plugin, d->mCustomPages ) {
      plugin->storeContact( contact );
    }
  }
}

void ContactEditorWidget::setReadOnly( bool readOnly )
{
  // widgets from name group
  d->mNameWidget->setReadOnly( readOnly );
  d->mPhotoWidget->setReadOnly( readOnly );
  d->mDisplayNameWidget->setReadOnly( readOnly );
  d->mNickNameWidget->setReadOnly( readOnly );
  d->mPronunciationWidget->setReadOnly( readOnly );

  // widgets from Internet group
  d->mEmailWidget->setReadOnly( readOnly );
  d->mHomepageWidget->setReadOnly( readOnly );
  d->mBlogWidget->setReadOnly( readOnly );
  d->mIMWidget->setReadOnly( readOnly );

  // widgets from phones group
  d->mPhonesWidget->setReadOnly( readOnly );

  // widgets from categories section
  d->mCategoriesWidget->setReadOnly( readOnly );

  // Preferred Mail formatting option
  d->mMailPreferFormatting->setEnabled( !readOnly );
  d->mAllowRemoteContent->setEnabled( !readOnly );

  // widgets from addresses group
  d->mAddressesWidget->setReadOnly( readOnly );

  // widgets from coordinates group
  d->mCoordinatesWidget->setReadOnly( readOnly );

  // widgets from general group
  d->mLogoWidget->setReadOnly( readOnly );
  d->mOrganizationWidget->setReadOnly( readOnly );
  d->mProfessionWidget->setReadOnly( readOnly );
  d->mTitleWidget->setReadOnly( readOnly );
  d->mDepartmentWidget->setReadOnly( readOnly );
  d->mOfficeWidget->setReadOnly( readOnly );
  d->mManagerWidget->setReadOnly( readOnly );
  d->mAssistantWidget->setReadOnly( readOnly );

  // widgets from groupware group
  d->mFreeBusyWidget->setReadOnly( readOnly );

  // widgets from notes group
  d->mNotesWidget->setReadOnly( readOnly );

  // widgets from dates group
  d->mBirthdateWidget->setReadOnly( readOnly );
  d->mAnniversaryWidget->setReadOnly( readOnly );

  // widgets from family group
  d->mPartnerWidget->setReadOnly( readOnly );

  if (d->mDisplayMode == FullMode) {
    // widgets from custom fields group
    d->mCustomFieldsWidget->setReadOnly( readOnly );

    // custom pages
    foreach ( Akonadi::ContactEditorPagePlugin *plugin, d->mCustomPages ) {
      plugin->setReadOnly( readOnly );
    }
  }
}
