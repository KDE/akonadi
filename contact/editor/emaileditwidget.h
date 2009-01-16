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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef EMAILEDITWIDGET_H
#define EMAILEDITWIDGET_H

#include <kdialog.h>

namespace KABC
{
class Addressee;
}

class KLineEdit;
class KListWidget;
class QToolButton;

/**
 * @short A widget for editing email addresses.
 *
 * The widget will show the preferred email address in a lineedit
 * and provides a button to open a dialog for further editing.
 */
class EmailEditWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit EmailEditWidget( QWidget *parent = 0 );
    ~EmailEditWidget();

    void loadContact( const KABC::Addressee &contact );
    void storeContact( KABC::Addressee &contact ) const;

    void setReadOnly( bool readOnly );

  private Q_SLOTS:
    void edit();
    void textChanged( const QString& );

  private:
    KLineEdit *mEmailEdit;
    QToolButton *mEditButton;
    QStringList mEmailList;
};

class EmailEditDialog : public KDialog
{
  Q_OBJECT

  public:
    EmailEditDialog( const QStringList &list, QWidget *parent = 0 );
    ~EmailEditDialog();

    QStringList emails() const;
    bool changed() const;

  protected Q_SLOTS:
    void add();
    void edit();
    void remove();
    void standard();
    void selectionChanged();

  private:
    KListWidget *mEmailListBox;
    QPushButton *mAddButton;
    QPushButton *mRemoveButton;
    QPushButton *mEditButton;
    QPushButton *mStandardButton;

    bool mChanged;
};

#endif
