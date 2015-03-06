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

#include "addresseditwidget.h"

#include "autoqpointer_p.h"

#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

#include <kacceleratormanager.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <khbox.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <klocalizedstring.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kseparator.h>
#include <ktextedit.h>

#include <functional>

struct LocaleAwareLessThan : std::binary_function<QString, QString, bool> {
    bool operator()(const QString &s1, const QString &s2) const
    {
        return QString::localeAwareCompare(s1, s2) < 0 ;
    }
};

class TabPressEater : public QObject
{
public:
    TabPressEater(QObject *parent)
        : QObject(parent)
    {
        setObjectName(QLatin1String("TabPressEater"));
    }

protected:
    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = (QKeyEvent *)event;
            if (keyEvent->key() == Qt::Key_Tab) {
                QApplication::sendEvent(parent(), event);
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
};

/**
 * Dialog for creating a new address types.
 *
 * @note This dialog is only used by AddressTypeCombo.
 */
class AddressTypeDialog : public KDialog
{
public:
    AddressTypeDialog(KABC::Address::Type type, QWidget *parent);
    ~AddressTypeDialog();

    KABC::Address::Type type() const;

private:
    QButtonGroup *mGroup;

    KABC::Address::TypeList mTypeList;
};

AddressSelectionWidget::AddressSelectionWidget(QWidget *parent)
    : KComboBox(parent)
{
    connect(this, SIGNAL(activated(int)), SLOT(selected(int)));
}

AddressSelectionWidget::~AddressSelectionWidget()
{
}

void AddressSelectionWidget::setAddresses(const KABC::Address::List &addresses)
{
    mAddresses = addresses;
    updateView();
}

void AddressSelectionWidget::setCurrentAddress(const KABC::Address &address)
{
    const int index = mAddresses.indexOf(address);
    if (index != -1) {
        setCurrentIndex(index);
    }
}

KABC::Address AddressSelectionWidget::currentAddress() const
{
    if (currentIndex() != -1 && currentIndex() < mAddresses.count()) {
        return mAddresses.at(currentIndex());
    } else {
        return KABC::Address();
    }
}

void AddressSelectionWidget::selected(int index)
{
    Q_ASSERT(index != -1 && index < mAddresses.count());
    emit selectionChanged(mAddresses.at(index));
}

void AddressSelectionWidget::updateView()
{
    clear();
    for (int i = 0; i < mAddresses.count(); ++i) {
        addItem(KABC::Address::typeLabel(mAddresses.at(i).type()));
    }
}

AddressTypeCombo::AddressTypeCombo(QWidget *parent)
    : KComboBox(parent)
    , mType(KABC::Address::Home)
    , mLastSelected(0)
{
    for (int i = 0; i < KABC::Address::typeList().count(); ++i) {
        mTypeList.append(KABC::Address::typeList().at(i));
    }
    mTypeList.append(-1);   // Others...

    update();

    connect(this, SIGNAL(activated(int)),
            this, SLOT(selected(int)));
}

AddressTypeCombo::~AddressTypeCombo()
{
}

void AddressTypeCombo::setType(KABC::Address::Type type)
{
    if (!mTypeList.contains((int)type)) {
        // insert at the end, but before the 'Others...' entry
        mTypeList.insert(mTypeList.at(mTypeList.count() - 1), (int)type);
    }

    mType = type;
    update();
}

KABC::Address::Type AddressTypeCombo::type() const
{
    return mType;
}

void AddressTypeCombo::update()
{
    bool blocked = signalsBlocked();
    blockSignals(true);

    clear();
    for (int i = 0; i < mTypeList.count(); ++i) {
        if (mTypeList.at(i) == -1) {     // "Other..." entry
            addItem(i18nc("@item:inlistbox Category of contact info field", "Other..."));
        } else {
            addItem(KABC::Address::typeLabel(KABC::Address::Type(mTypeList.at(i))));
        }
    }

    setCurrentIndex(mLastSelected = mTypeList.indexOf(mType));

    blockSignals(blocked);
}

void AddressTypeCombo::selected(int pos)
{
    if (mTypeList.at(pos) == -1) {
        otherSelected();
    } else {
        mType = KABC::Address::Type(mTypeList.at(pos));
        mLastSelected = pos;
    }
}

void AddressTypeCombo::otherSelected()
{
    AutoQPointer<AddressTypeDialog> dlg = new AddressTypeDialog(mType, this);
    if (dlg->exec()) {
        mType = dlg->type();
        if (!mTypeList.contains(mType)) {
            mTypeList.insert(mTypeList.at(mTypeList.count() - 1), mType);
        }
    } else {
        setType(KABC::Address::Type(mTypeList.at(mLastSelected)));
    }

    update();
}

AddressEditWidget::AddressEditWidget(QWidget *parent)
    : QWidget(parent)
    , mReadOnly(false)
{
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(KDialog::spacingHint());
    layout->setMargin(0);

    QHBoxLayout *hboxLayout = new QHBoxLayout;
    QLabel *label = new QLabel(i18nc("@label:listbox type of address", "Address type:"), this);
    hboxLayout->addWidget(label);

    mAddressSelectionWidget = new AddressSelectionWidget(this);
    connect(mAddressSelectionWidget, SIGNAL(selectionChanged(KABC::Address)),
            SLOT(updateAddressView()));
    label->setBuddy(mAddressSelectionWidget);
    hboxLayout->addWidget(mAddressSelectionWidget, 1);
    layout->addLayout(hboxLayout, 0, 0, 1, 3);

    mAddressView = new QLabel(this);
    mAddressView->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    mAddressView->setMinimumHeight(20);
    mAddressView->setAlignment(Qt::AlignTop);
    mAddressView->setTextFormat(Qt::PlainText);
    mAddressView->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    layout->addWidget(mAddressView, 1, 0, 1, 3);

    mCreateButton = new QPushButton(i18nc("@action:button street/postal", "New..."), this);
    connect(mCreateButton, SIGNAL(clicked()), this, SLOT(createAddress()));
    mEditButton = new QPushButton(i18nc("@action:button street/postal", "Edit..."), this);
    connect(mEditButton, SIGNAL(clicked()), this, SLOT(editAddress()));
    mDeleteButton = new QPushButton(i18nc("@action:button street/postal", "Delete"), this);
    connect(mDeleteButton, SIGNAL(clicked()), this, SLOT(deleteAddress()));

    layout->addWidget(mCreateButton, 2, 0);
    layout->addWidget(mEditButton, 2, 1);
    layout->addWidget(mDeleteButton, 2, 2);
    setLayout(layout);
    updateButtons();
}

AddressEditWidget::~AddressEditWidget()
{
}

void AddressEditWidget::setReadOnly(bool readOnly)
{
    if (mReadOnly != readOnly) {
        mReadOnly = readOnly;
        updateButtons();
    }
}

void AddressEditWidget::updateName(const QString &name)
{
    if (mName != name) {
        mName = name;
        updateAddressView();
    }
}

void AddressEditWidget::createAddress()
{
    AutoQPointer<AddressEditDialog> dialog = new AddressEditDialog(this);
    if (dialog->exec()) {
        const KABC::Address address = dialog->address();
        fixPreferredAddress(address);
        mAddressList.append(address);
        mAddressSelectionWidget->setAddresses(mAddressList);
        mAddressSelectionWidget->setCurrentAddress(address);

        updateAddressView();
        updateButtons();
    }
}

void AddressEditWidget::editAddress()
{
    AutoQPointer<AddressEditDialog> dialog = new AddressEditDialog(this);
    dialog->setAddress(mAddressSelectionWidget->currentAddress());
    if (dialog->exec()) {
        const KABC::Address address = dialog->address();
        fixPreferredAddress(address);
        mAddressList[mAddressSelectionWidget->currentIndex()] = address;
        mAddressSelectionWidget->setAddresses(mAddressList);
        mAddressSelectionWidget->setCurrentAddress(address);

        updateAddressView();
    }
}

void AddressEditWidget::deleteAddress()
{
    const int result = KMessageBox::questionYesNo(this, i18n("Do you really want to delete this address?"));

    if (result != KMessageBox::Yes) {
        return;
    }

    mAddressList.removeAt(mAddressSelectionWidget->currentIndex());
    mAddressSelectionWidget->setAddresses(mAddressList);
    updateAddressView();
    updateButtons();
}

void AddressEditWidget::fixPreferredAddress(const KABC::Address &preferredAddress)
{
    // as the preferred address is mutual exclusive, we have to
    // remove the flag from all other addresses
    if (preferredAddress.type() &KABC::Address::Pref) {
        for (int i = 0; i < mAddressList.count(); ++i) {
            KABC::Address &address = mAddressList[i];
            address.setType(address.type() &~KABC::Address::Pref);
        }
    }
}

void AddressEditWidget::updateAddressView()
{
    const KABC::Address address = mAddressSelectionWidget->currentAddress();

    if (address.isEmpty()) {
        mAddressView->setText(QString());
    } else {
        mAddressView->setText(address.formattedAddress(mName));
    }
}

void AddressEditWidget::updateButtons()
{
    mCreateButton->setEnabled(!mReadOnly);
    mEditButton->setEnabled(!mReadOnly && (mAddressList.count() > 0));
    mDeleteButton->setEnabled(!mReadOnly && (mAddressList.count() > 0));
}

void AddressEditWidget::loadContact(const KABC::Addressee &contact)
{
    mName = contact.realName();
    mAddressList = contact.addresses();

    mAddressSelectionWidget->setAddresses(mAddressList);

    // set the preferred address as the visible one
    for (int i = 0; i < mAddressList.count(); ++i) {
        if (mAddressList.at(i).type() &KABC::Address::Pref) {
            mAddressSelectionWidget->setCurrentAddress(mAddressList.at(i));
            break;
        }
    }

    updateAddressView();
    updateButtons();
}

void AddressEditWidget::storeContact(KABC::Addressee &contact) const
{
    // delete all previous addresses
    const KABC::Address::List oldAddresses = contact.addresses();
    for (int i = 0; i < oldAddresses.count(); ++i) {
        contact.removeAddress(oldAddresses.at(i));
    }

    // insert the new ones
    for (int i = 0; i < mAddressList.count(); ++i) {
        const KABC::Address address(mAddressList.at(i));
        if (!address.isEmpty()) {
            contact.insertAddress(address);
        }
    }
}

AddressEditDialog::AddressEditDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18nc("street/postal", "Edit Address"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QGridLayout *topLayout = new QGridLayout(page);
    topLayout->setSpacing(spacingHint());
    topLayout->setMargin(0);

    QLabel *label = new QLabel(i18nc("@label:listbox type of address", "Address type:"), this);
    topLayout->addWidget(label, 0, 0);

    mTypeCombo = new AddressTypeCombo(page);
    label->setBuddy(mTypeCombo);
    topLayout->addWidget(mTypeCombo, 0, 1);

    label = new QLabel(i18nc("<streetLabel>:", "%1:", KABC::Address::streetLabel()), page);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    topLayout->addWidget(label, 1, 0);
    mStreetTextEdit = new KTextEdit(page);
    mStreetTextEdit->setAcceptRichText(false);
    label->setBuddy(mStreetTextEdit);
    topLayout->addWidget(mStreetTextEdit, 1, 1);

    TabPressEater *eater = new TabPressEater(this);
    mStreetTextEdit->installEventFilter(eater);

    label = new QLabel(i18nc("<postOfficeBoxLabel>:", "%1:", KABC::Address::postOfficeBoxLabel()), page);
    topLayout->addWidget(label, 2 , 0);
    mPOBoxEdit = new KLineEdit(page);
    mPOBoxEdit->setTrapReturnKey(true);
    label->setBuddy(mPOBoxEdit);
    topLayout->addWidget(mPOBoxEdit, 2, 1);

    label = new QLabel(i18nc("<localityLabel>:", "%1:", KABC::Address::localityLabel()), page);
    topLayout->addWidget(label, 3, 0);
    mLocalityEdit = new KLineEdit(page);
    mLocalityEdit->setTrapReturnKey(true);
    label->setBuddy(mLocalityEdit);
    topLayout->addWidget(mLocalityEdit, 3, 1);

    label = new QLabel(i18nc("<regionLabel>:", "%1:", KABC::Address::regionLabel()), page);
    topLayout->addWidget(label, 4, 0);
    mRegionEdit = new KLineEdit(page);
    mRegionEdit->setTrapReturnKey(true);
    label->setBuddy(mRegionEdit);
    topLayout->addWidget(mRegionEdit, 4, 1);

    label = new QLabel(i18nc("<postalCodeLabel>:", "%1:", KABC::Address::postalCodeLabel()), page);
    topLayout->addWidget(label, 5, 0);
    mPostalCodeEdit = new KLineEdit(page);
    mPostalCodeEdit->setTrapReturnKey(true);
    label->setBuddy(mPostalCodeEdit);
    topLayout->addWidget(mPostalCodeEdit, 5, 1);

    label = new QLabel(i18nc("<countryLabel>:", "%1:", KABC::Address::countryLabel()), page);
    topLayout->addWidget(label, 6, 0);
    mCountryCombo = new KComboBox(page);
    mCountryCombo->setEditable(true);
    mCountryCombo->setDuplicatesEnabled(false);

    QPushButton *labelButton = new QPushButton(i18n("Edit Label..."), page);
    topLayout->addWidget(labelButton, 7, 0, 1, 2);
    connect(labelButton, SIGNAL(clicked()), SLOT(editLabel()));

    fillCountryCombo();
    label->setBuddy(mCountryCombo);
    topLayout->addWidget(mCountryCombo, 6, 1);

    mPreferredCheckBox = new QCheckBox(i18nc("street/postal", "This is the preferred address"), page);
    topLayout->addWidget(mPreferredCheckBox, 8, 0, 1, 2);

    KHBox *buttonBox = new KHBox(page);
    buttonBox->setSpacing(spacingHint());
    topLayout->addWidget(buttonBox, 9, 0, 1, 2);

    KAcceleratorManager::manage(this);
}

AddressEditDialog::~AddressEditDialog()
{
}

void AddressEditDialog::editLabel()
{
    bool ok = false;
    QString result = KInputDialog::getMultiLineText(KABC::Address::labelLabel(),
                                                    KABC::Address::labelLabel(),
                                                    mLabel, &ok, this);
    if (ok) {
        mLabel = result;
    }
}

void AddressEditDialog::setAddress(const KABC::Address &address)
{
    mAddress = address;

    mTypeCombo->setType(mAddress.type());
    mStreetTextEdit->setPlainText(mAddress.street());
    mRegionEdit->setText(mAddress.region());
    mLocalityEdit->setText(mAddress.locality());
    mPostalCodeEdit->setText(mAddress.postalCode());
    mPOBoxEdit->setText(mAddress.postOfficeBox());
    mLabel = mAddress.label();
    mPreferredCheckBox->setChecked(mAddress.type() &KABC::Address::Pref);

    if (mAddress.isEmpty()) {
        mCountryCombo->setItemText(mCountryCombo->currentIndex(),
                                   KGlobal::locale()->countryCodeToName(KGlobal::locale()->country()));
    } else {
        mCountryCombo->setItemText(mCountryCombo->currentIndex(), mAddress.country());
    }

    mStreetTextEdit->setFocus();
}

KABC::Address AddressEditDialog::address() const
{
    KABC::Address address(mAddress);

    address.setType(mTypeCombo->type());
    address.setLocality(mLocalityEdit->text());
    address.setRegion(mRegionEdit->text());
    address.setPostalCode(mPostalCodeEdit->text());
    address.setCountry(mCountryCombo->currentText());
    address.setPostOfficeBox(mPOBoxEdit->text());
    address.setStreet(mStreetTextEdit->toPlainText());
    address.setLabel(mLabel);

    if (mPreferredCheckBox->isChecked()) {
        address.setType(address.type() | KABC::Address::Pref);
    } else {
        address.setType(address.type() &~(KABC::Address::Pref));
    }

    return address;
}

void AddressEditDialog::fillCountryCombo()
{
    QStringList countries;

    foreach (const QString &cc, KGlobal::locale()->allCountriesList()) {
        countries.append(KGlobal::locale()->countryCodeToName(cc));
    }

    qSort(countries.begin(), countries.end(), LocaleAwareLessThan());

    mCountryCombo->addItems(countries);
    mCountryCombo->setAutoCompletion(true);
    mCountryCombo->completionObject()->setItems(countries);
    mCountryCombo->completionObject()->setIgnoreCase(true);

    const QString currentCountry = KGlobal::locale()->countryCodeToName(KGlobal::locale()->country());
    mCountryCombo->setCurrentIndex(mCountryCombo->findText(currentCountry));
}

AddressTypeDialog::AddressTypeDialog(KABC::Address::Type type, QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18nc("street/postal", "Edit Address Type"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget *page = new QWidget(this);
    setMainWidget(page);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(KDialog::spacingHint());
    layout->setMargin(0);

    QGroupBox *box  = new QGroupBox(i18nc("street/postal", "Address Types"), page);
    layout->addWidget(box);
    mGroup = new QButtonGroup(box);
    mGroup->setExclusive(false);

    QGridLayout *buttonLayout = new QGridLayout(box);

    mTypeList = KABC::Address::typeList();
    mTypeList.removeAll(KABC::Address::Pref);

    KABC::Address::TypeList::ConstIterator it;
    int i = 0;
    int row = 0;
    for (it = mTypeList.constBegin(); it != mTypeList.constEnd(); ++it, ++i) {
        QCheckBox *cb = new QCheckBox(KABC::Address::typeLabel(*it), box);
        cb->setChecked(type &mTypeList[i]);
        buttonLayout->addWidget(cb, row, i % 3);

        if (i % 3 == 2) {
            ++row;
        }
        mGroup->addButton(cb);
    }
}

AddressTypeDialog::~AddressTypeDialog()
{
}

KABC::Address::Type AddressTypeDialog::type() const
{
    KABC::Address::Type type;
    for (int i = 0; i < mGroup->buttons().count(); ++i) {
        QCheckBox *box = dynamic_cast<QCheckBox *>(mGroup->buttons().at(i));
        if (box && box->isChecked()) {
            type |= mTypeList[i];
        }
    }

    return type;
}
