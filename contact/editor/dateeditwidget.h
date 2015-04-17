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

#ifndef DATEEDITWIDGET_H
#define DATEEDITWIDGET_H

#include <QtCore/QDate>
#include <QLabel>
#include <QWidget>

class KDatePickerPopup;

class QContextMenuEvent;
class QToolButton;

class DateView : public QLabel
{
    Q_OBJECT

public:
    explicit DateView(QWidget *parent = 0);

Q_SIGNALS:
    void resetDate();

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private Q_SLOTS:
    void emitSignal();
};

class DateEditWidget : public QWidget
{
    Q_OBJECT

public:
    enum Type {
        General,
        Birthday,
        Anniversary
    };

    explicit DateEditWidget(Type type = General, QWidget *parent = 0);
    ~DateEditWidget();

    void setDate(const QDate &date);
    QDate date() const;

    void setReadOnly(bool readOnly);

private Q_SLOTS:
    void dateSelected(const QDate &date);
    void resetDate();
    void updateView();

private:
    DateView *mView;
    QToolButton *mSelectButton;
    QToolButton *mClearButton;
    KDatePickerPopup *mMenu;
    QDate mDate;
    bool mReadOnly;
};

#endif
