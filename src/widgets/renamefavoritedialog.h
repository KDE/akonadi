/*
    Copyright (c) 2011 Laurent Montel <montel@kde.org>

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

#ifndef RENAMEFAVORITEDIALOG_H
#define RENAMEFAVORITEDIALOG_H

#include <QDialog>
#include "collection.h"

class QLabel;
class QLineEdit;
class QPushButton;

class RenameFavoriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RenameFavoriteDialog(const QString &caption, const QString &text, const QString &value, const QString &defaultName, QWidget *parent);
    ~RenameFavoriteDialog();

    QString newName() const;

protected:

protected Q_SLOTS:
    void slotEditTextChanged(const QString &);
    void slotDefaultName();
private:
    QString m_defaultName;
    QLabel *m_label;
    QLineEdit *m_lineEdit;
    QPushButton *mOkButton;
};

#endif /* RENAMEFAVORITEDIALOG_H */
