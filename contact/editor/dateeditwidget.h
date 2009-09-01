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

#ifndef DATEEDITWIDGET_H
#define DATEEDITWIDGET_H

#include <QtCore/QDate>
#include <QtGui/QLabel>
#include <QtGui/QWidget>

namespace KPIM
{
class KDatePickerPopup;
}

class QContextMenuEvent;
class QToolButton;

class DateView : public QLabel
{
  Q_OBJECT

  public:
    DateView( QWidget *parent = 0 );

  Q_SIGNALS:
    void resetDate();

  protected:
    virtual void contextMenuEvent( QContextMenuEvent* );

  private Q_SLOTS:
    void emitSignal();
};

class DateEditWidget : public QWidget
{
  Q_OBJECT

  public:
    DateEditWidget( QWidget *parent = 0 );
    ~DateEditWidget();

    void setDate( const QDate &date );
    QDate date() const;

    void setReadOnly( bool readOnly );

  private Q_SLOTS:
    void dateSelected( const QDate& );
    void resetDate();
    void updateView();

  private:
    QDate mDate;
    DateView *mView;
    QToolButton *mSelectButton;
    QToolButton *mClearButton;
    KPIM::KDatePickerPopup *mMenu;
    bool mReadOnly;
};

#endif
