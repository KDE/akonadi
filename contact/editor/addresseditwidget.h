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

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef ADDRESSEDITWIDGET_H
#define ADDRESSEDITWIDGET_H

#include <QtGui/QWidget>

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kcombobox.h>
#include <kdialog.h>

class QCheckBox;
class QLabel;

class KLineEdit;
class KTextEdit;

/**
 * @short A widget that shows a list of addresses for selection.
 */
class AddressSelectionWidget : public KComboBox
{
  Q_OBJECT

  public:
    /**
     * Creates a new address selection widget.
     *
     * @param parent The parent widget.
     */
    AddressSelectionWidget( QWidget *parent = 0 );

    /**
     * Destroys the address selection widget.
     */
    virtual ~AddressSelectionWidget();

    /**
     * Sets the list of @p addresses that can be chosen from.
     */
    void setAddresses( const KABC::Address::List &addresses );

    /**
     * Sets the current @p address.
     */
    void setCurrentAddress( const KABC::Address &address );

    /**
     * Returns the current selected address.
     */
    KABC::Address currentAddress() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the selection of the
     * address has changed.
     *
     * @param address The new selected address.
     */
    void selectionChanged( const KABC::Address &address );

  private Q_SLOTS:
    void selected( int );

  private:
    void updateView();

    KABC::Address::List mAddresses;
};

/**
 * @short A widget for selecting the type of an address.
 */
class AddressTypeCombo : public KComboBox
{
  Q_OBJECT

  public:
    /**
     * Creates a new address type combo.
     *
     * @param parent The parent widget.
     */
    AddressTypeCombo( QWidget *parent = 0 );

    /**
     * Destroys the address type combo.
     */
    ~AddressTypeCombo();

    /**
     * Sets the type that shall be selected in the combobox.
     */
    void setType( KABC::Address::Type type );

    /**
     * Returns the type that is currently selected.
     */
    KABC::Address::Type type() const;

  private Q_SLOTS:
    void selected( int );
    void otherSelected();

  private:
    void update();

    KABC::Address::Type mType;
    int mLastSelected;
    QList<int> mTypeList;
};

/**
 * @short An editor widget for addresses.
 */
class AddressEditWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit AddressEditWidget( QWidget *parent = 0 );
    ~AddressEditWidget();

    void loadContact( const KABC::Addressee &contact );
    void storeContact( KABC::Addressee &contact ) const;

    void setReadOnly( bool readOnly );

  public Q_SLOTS:
    void updateName( const QString &name );

  private Q_SLOTS:
    void updateAddressView();
    void createAddress();
    void editAddress();
    void deleteAddress();

  private:
    void updateButtons();
    void fixPreferredAddress( const KABC::Address &preferredAddress );

    AddressSelectionWidget *mAddressSelectionWidget;

    QLabel *mAddressView;
    QPushButton *mCreateButton;
    QPushButton *mEditButton;
    QPushButton *mDeleteButton;

    KABC::Address::List mAddressList;
    QString mName;
    bool mReadOnly;
};

/**
  Dialog for editing address details.
 */
class AddressEditDialog : public KDialog
{
  Q_OBJECT

  public:
    AddressEditDialog( QWidget *parent = 0 );
    ~AddressEditDialog();

    void setAddress( const KABC::Address &address );
    KABC::Address address() const;

  private Q_SLOTS:
    void editLabel();

  private:
    void fillCountryCombo();
    QStringList sortLocaleAware( const QStringList& );

    AddressTypeCombo *mTypeCombo;
    KTextEdit *mStreetTextEdit;
    KComboBox *mCountryCombo;
    KLineEdit *mRegionEdit;
    KLineEdit *mLocalityEdit;
    KLineEdit *mPostalCodeEdit;
    KLineEdit *mPOBoxEdit;
    QCheckBox *mPreferredCheckBox;

    KABC::Address mAddress;
    QString mLabel;
};

#endif
