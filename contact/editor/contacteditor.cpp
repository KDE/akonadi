/*
    This file is part of KContactManager.

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

#include "contacteditor.h"

#include "addresseditwidget.h"
#include "dateeditwidget.h"
#include "displaynameeditwidget.h"
#include "emaileditwidget.h"
#include "imagewidget.h"
#include "nameeditwidget.h"
#include "phoneeditwidget.h"
#include "soundeditwidget.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klineedit.h>
#include <klocale.h>
#include <ktabwidget.h>
#include <kurlrequester.h>
#include <ktextedit.h>

#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

class ContactEditor::Private
{
  public:
    Private( ContactEditor *parent )
      : mParent( parent )
    {
    }

    void initGui();
    void initGuiContactTab();
    void initGuiLocationTab();
    void initGuiBusinessTab();
    void initGuiPersonalTab();

    QString loadCustom( const KABC::Addressee &contact, const QString &key ) const;
    void storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const;

    ContactEditor *mParent;
    KTabWidget *mTabWidget;

    // widgets from name group
    NameEditWidget *mNameWidget;
    ImageWidget *mPhotoWidget;
    DisplayNameEditWidget *mDisplayNameWidget;
    KLineEdit *mNickNameWidget;
    SoundEditWidget *mPronunciationWidget;

    // widgets from internet group
    EmailEditWidget *mEmailWidget;
    KLineEdit *mHomepageWidget;
    KLineEdit *mBlogWidget;
    KLineEdit *mIMWidget;

    // widgets from phones group
    PhoneEditWidget *mPhonesWidget;

    // widgets from addresses group
    AddressEditWidget *mAddressesWidget;

    // widgets from coordinates group
    QWidget *mCoordinatesWidget;

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
    KUrlRequester *mFreeBusyWidget;

    // widgets from notes group
    KTextEdit *mNotesWidget;

    // widgets from dates group
    DateEditWidget *mBirthdateWidget;
    DateEditWidget *mAnniversaryWidget;

    // widgets from family group
    KLineEdit *mPartnerWidget;
};

void ContactEditor::Private::initGui()
{
  QVBoxLayout *layout = new QVBoxLayout( mParent );
  layout->setMargin( 0 );

  mTabWidget = new KTabWidget( mParent );
  layout->addWidget( mTabWidget );

  initGuiContactTab();
  initGuiLocationTab();
  initGuiBusinessTab();
  initGuiPersonalTab();
}

void ContactEditor::Private::initGuiContactTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Contact" ) );

  QGroupBox *nameGroupBox = new QGroupBox( i18n( "Name" ) );
  QGroupBox *internetGroupBox = new QGroupBox( i18n( "Internet" ) );
  QGroupBox *phonesGroupBox = new QGroupBox( i18n( "Phones" ) );

  layout->addWidget( nameGroupBox );
  layout->addWidget( internetGroupBox );
  layout->addWidget( phonesGroupBox );

  QGridLayout *nameLayout = new QGridLayout( nameGroupBox );
  QGridLayout *internetLayout = new QGridLayout( internetGroupBox );
  QGridLayout *phonesLayout = new QGridLayout( phonesGroupBox );

  QLabel *label = 0;

  // setup name group box
  label = new QLabel( i18n( "Name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 0, 0 );

  mNameWidget = new NameEditWidget;
  label->setBuddy( mNameWidget );
  nameLayout->addWidget( mNameWidget, 0, 1 );

  mPhotoWidget = new ImageWidget( ImageWidget::Photo );
  mPhotoWidget->setMinimumSize( QSize( 100, 140 ) );
  nameLayout->addWidget( mPhotoWidget, 0, 2, 4, 1 );

  label = new QLabel( i18n( "Display:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 1, 0 );

  mDisplayNameWidget = new DisplayNameEditWidget;
  label->setBuddy( mDisplayNameWidget );
  nameLayout->addWidget( mDisplayNameWidget, 1, 1 );

  label = new QLabel( i18n( "Nickname:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 2, 0 );

  mNickNameWidget = new KLineEdit;
  label->setBuddy( mNickNameWidget );
  nameLayout->addWidget( mNickNameWidget, 2, 1 );

  label = new QLabel( i18n( "Pronunciation:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 3, 0 );

  mPronunciationWidget = new SoundEditWidget;
  label->setBuddy( mPronunciationWidget );
  nameLayout->addWidget( mPronunciationWidget, 3, 1 );

  nameLayout->setRowStretch( 4, 1 );

  // setup internet group box
  label = new QLabel( i18n( "Email:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 0, 0 );

  mEmailWidget = new EmailEditWidget;
  label->setBuddy( mEmailWidget );
  internetLayout->addWidget( mEmailWidget, 0, 1 );

  label = new QLabel( i18n( "Homepage:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 1, 0 );

  mHomepageWidget = new KLineEdit;
  label->setBuddy( mHomepageWidget );
  internetLayout->addWidget( mHomepageWidget, 1, 1 );

  label = new QLabel( i18n( "Blog:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 2, 0 );

  mBlogWidget = new KLineEdit;
  label->setBuddy( mBlogWidget );
  internetLayout->addWidget( mBlogWidget, 2, 1 );

  label = new QLabel( i18n( "Messaging:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 3, 0 );

  mIMWidget = new KLineEdit;
  label->setBuddy( mIMWidget );
  internetLayout->addWidget( mIMWidget, 3, 1 );

  internetLayout->setRowStretch( 4, 1 );

  // setup phones group box
  mPhonesWidget = new PhoneEditWidget;
  phonesLayout->addWidget( mPhonesWidget, 0, 0 );

  phonesLayout->setRowStretch( 1, 1 );
}

void ContactEditor::Private::initGuiLocationTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Location" ) );

  QGroupBox *addressesGroupBox = new QGroupBox( i18n( "Addresses" ) );
  QGroupBox *coordinatesGroupBox = new QGroupBox( i18n( "Coordinates" ) );

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
  mCoordinatesWidget = new QWidget;
  coordinatesLayout->addWidget( mCoordinatesWidget, 0, 0 );
  coordinatesLayout->setRowStretch( 1, 1 );
}

void ContactEditor::Private::initGuiBusinessTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Business" ) );

  QGroupBox *generalGroupBox = new QGroupBox( i18n( "General" ) );
  QGroupBox *groupwareGroupBox = new QGroupBox( i18n( "Groupware" ) );
  QGroupBox *notesGroupBox = new QGroupBox( i18n( "Notes" ) );

  layout->addWidget( generalGroupBox );
  layout->addWidget( groupwareGroupBox );
  layout->addWidget( notesGroupBox );

  QGridLayout *generalLayout = new QGridLayout( generalGroupBox );
  QGridLayout *groupwareLayout = new QGridLayout( groupwareGroupBox );
  QGridLayout *notesLayout = new QGridLayout( notesGroupBox );

  QLabel *label = 0;

  // setup general group box
  mLogoWidget = new ImageWidget( ImageWidget::Logo );
  generalLayout->addWidget( mLogoWidget, 0, 2, 6, 1, Qt::AlignTop );

  label = new QLabel( i18n( "Organization:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 0, 0 );

  mOrganizationWidget = new KLineEdit;
  label->setBuddy( mOrganizationWidget );
  generalLayout->addWidget( mOrganizationWidget, 0, 1 );

  label = new QLabel( i18n( "Profession:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 1, 0 );

  mProfessionWidget = new KLineEdit;
  label->setBuddy( mProfessionWidget );
  generalLayout->addWidget( mProfessionWidget, 1, 1 );

  label = new QLabel( i18n( "Title:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 2, 0 );

  mTitleWidget = new KLineEdit;
  label->setBuddy( mTitleWidget );
  generalLayout->addWidget( mTitleWidget , 2, 1 );

  label = new QLabel( i18n( "Department:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 3, 0 );

  mDepartmentWidget = new KLineEdit;
  label->setBuddy( mDepartmentWidget );
  generalLayout->addWidget( mDepartmentWidget, 3, 1 );

  label = new QLabel( i18n( "Office:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 4, 0 );

  mOfficeWidget = new KLineEdit;
  label->setBuddy( mOfficeWidget );
  generalLayout->addWidget( mOfficeWidget, 4, 1 );

  label = new QLabel( i18n( "Manager's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 5, 0 );

  mManagerWidget = new KLineEdit;
  label->setBuddy( mManagerWidget );
  generalLayout->addWidget( mManagerWidget, 5, 1 );

  label = new QLabel( i18n( "Assistant's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 6, 0 );

  mAssistantWidget = new KLineEdit;
  label->setBuddy( mAssistantWidget );
  generalLayout->addWidget( mAssistantWidget, 6, 1 );

  generalLayout->setRowStretch( 7, 1 );

  // setup groupware group box
  label = new QLabel( i18n( "Free/Busy:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  groupwareLayout->addWidget( label, 0, 0 );

  mFreeBusyWidget = new KUrlRequester;
  label->setBuddy( mFreeBusyWidget );
  groupwareLayout->addWidget( mFreeBusyWidget, 0, 1 );

  groupwareLayout->setRowStretch( 1, 1 );

  // setup notes group box
  mNotesWidget = new KTextEdit;
  notesLayout->addWidget( mNotesWidget, 0, 0 );
}

void ContactEditor::Private::initGuiPersonalTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Personal" ) );

  QGroupBox *datesGroupBox = new QGroupBox( i18n( "Dates" ) );
  QGroupBox *familyGroupBox = new QGroupBox( i18n( "Family" ) );

  layout->addWidget( datesGroupBox );
  layout->addWidget( familyGroupBox );

  QGridLayout *datesLayout = new QGridLayout( datesGroupBox );
  QGridLayout *familyLayout = new QGridLayout( familyGroupBox );

  QLabel *label = 0;

  // setup dates group box
  label = new QLabel( i18n( "Birthdate:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 0, 0 );

  mBirthdateWidget = new DateEditWidget;
  label->setBuddy( mBirthdateWidget );
  datesLayout->addWidget( mBirthdateWidget, 0, 1 );

  label = new QLabel( i18n( "Anniversary:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 1, 0 );

  mAnniversaryWidget = new DateEditWidget;
  label->setBuddy( mAnniversaryWidget );
  datesLayout->addWidget( mAnniversaryWidget, 1, 1 );

  datesLayout->setRowStretch( 2, 1 );
  datesLayout->setColumnStretch( 1, 1 );

  // widgets from family group
  label = new QLabel( i18n( "Partner's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  familyLayout->addWidget( label, 0, 0 );

  mPartnerWidget = new KLineEdit;
  label->setBuddy( mPartnerWidget );
  familyLayout->addWidget( mPartnerWidget, 0, 1 );

  familyLayout->setRowStretch( 1, 1 );
}

QString ContactEditor::Private::loadCustom( const KABC::Addressee &contact, const QString &key ) const
{
  return contact.custom( "KADDRESSBOOK", key );
}

void ContactEditor::Private::storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const
{
  if ( value.isEmpty() )
    contact.removeCustom( "KADDRESSBOOK", key );
  else
    contact.insertCustom( "KADDRESSBOOK", key, value );
}

ContactEditor::ContactEditor( QWidget* )
  : d( new Private( this ) )
{
  d->initGui();

  KConfig config( "kcontactmanagerrc" );
  KConfigGroup group( &config, "General" );

  d->mDisplayNameWidget->setDisplayType( DisplayNameEditWidget::DisplayType( group.readEntry( "DisplayNameType", 1 ) ) );

  connect( d->mNameWidget, SIGNAL( nameChanged( const KABC::Addressee& ) ),
           d->mDisplayNameWidget, SLOT( changeName( const KABC::Addressee& ) ) );
  connect( d->mOrganizationWidget, SIGNAL( textChanged( const QString& ) ),
           d->mDisplayNameWidget, SLOT( changeOrganization( const QString& ) ) );
}

ContactEditor::~ContactEditor()
{
  KConfig config( "kcontactmanagerrc" );
  KConfigGroup group( &config, "General" );

  group.writeEntry( "DisplayNameType", (int)d->mDisplayNameWidget->displayType() );
}

void ContactEditor::loadContact( const KABC::Addressee &contact )
{
  // name group
  d->mPhotoWidget->loadContact( contact );
  d->mNameWidget->loadContact( contact );
  d->mDisplayNameWidget->loadContact( contact );
  d->mNickNameWidget->setText( contact.nickName() );
  d->mPronunciationWidget->loadContact( contact );

  // internet group
  d->mEmailWidget->loadContact( contact );
  d->mHomepageWidget->setUrl( contact.url() );
  d->mBlogWidget->setText( d->loadCustom( contact, "BlogFeed" ) );

  // phones group
  d->mPhonesWidget->loadContact( contact );

  // address group
  d->mAddressesWidget->loadContact( contact );

  // coordinates group

  // general group
  d->mLogoWidget->loadContact( contact );
  d->mOrganizationWidget->setText( contact.organization() );
  d->mProfessionWidget->setText( d->loadCustom( contact, "X-Profession" ) );
  d->mTitleWidget->setText( contact.title() );
  d->mDepartmentWidget->setText( contact.department() );
  d->mOfficeWidget->setText( d->loadCustom( contact, "X-Office" ) );
  d->mManagerWidget->setText( d->loadCustom( contact, "X-ManagersName" ) );
  d->mAssistantWidget->setText( d->loadCustom( contact, "X-AssistantsName" ) );

  // groupware group

  // notes group
  d->mNotesWidget->setPlainText( contact.note() );

  // dates group
  d->mBirthdateWidget->setDate( contact.birthday().date() );
  d->mAnniversaryWidget->setDate( QDate::fromString( d->loadCustom( contact, "X-Anniversary" ), Qt::ISODate ) );

  // family group
  d->mPartnerWidget->setText( d->loadCustom( contact, "X-SpousesName" ) );
}

void ContactEditor::storeContact( KABC::Addressee &contact ) const
{
  // name group
  d->mPhotoWidget->storeContact( contact );
  d->mNameWidget->storeContact( contact );
  d->mDisplayNameWidget->storeContact( contact );
  contact.setNickName( d->mNickNameWidget->text().trimmed() );
  d->mPronunciationWidget->storeContact( contact );

  // internet group
  d->mEmailWidget->storeContact( contact );
  contact.setUrl( KUrl( d->mHomepageWidget->text().trimmed() ) );
  d->storeCustom( contact, "BlogFeed", d->mBlogWidget->text().trimmed() );

  // phones group
  d->mPhonesWidget->storeContact( contact );

  // address group
  d->mAddressesWidget->storeContact( contact );

  // coordinates group

  // general group
  d->mLogoWidget->storeContact( contact );
  contact.setOrganization( d->mOrganizationWidget->text() );
  d->storeCustom( contact, "X-Profession", d->mProfessionWidget->text().trimmed() );
  contact.setTitle( d->mTitleWidget->text().trimmed() );
  contact.setDepartment( d->mDepartmentWidget->text().trimmed() );
  d->storeCustom( contact, "X-Office", d->mOfficeWidget->text().trimmed() );
  d->storeCustom( contact, "X-ManagersName", d->mManagerWidget->text().trimmed() );
  d->storeCustom( contact, "X-AssistantsName", d->mAssistantWidget->text().trimmed() );

  // groupware group

  // notes group
  contact.setNote( d->mNotesWidget->toPlainText() );

  // dates group
  contact.setBirthday( QDateTime( d->mBirthdateWidget->date(), QTime() ) );
  d->storeCustom( contact, "X-Anniversary", d->mAnniversaryWidget->date().toString( Qt::ISODate ) );

  // family group
  d->storeCustom( contact, "X-SpousesName", d->mPartnerWidget->text().trimmed() );
}
