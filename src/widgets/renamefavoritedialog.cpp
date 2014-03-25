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

#include "renamefavoritedialog.h"
#include <QLabel>
#include <KLineEdit>
#include <KLocalizedString>
#include <QVBoxLayout>

RenameFavoriteDialog::RenameFavoriteDialog(const QString &caption, const QString &text, const QString &value, const QString &defaultName, QWidget *parent)
    : KDialog(parent)
    , m_defaultName(defaultName)
{
    setCaption(caption);
    setButtons(Ok | Cancel | User1);
    setButtonGuiItem(User1, KGuiItem(i18n("Default Name")));
    setDefaultButton(Ok);
    setModal(true);

    QWidget *frame = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setMargin(0);

    m_label = new QLabel(text, frame);
    m_label->setWordWrap(true);
    layout->addWidget(m_label);

    m_lineEdit = new KLineEdit(value, frame);
    m_lineEdit->setClearButtonShown(true);
    layout->addWidget(m_lineEdit);

    m_lineEdit->setFocus();
    m_label->setBuddy(m_lineEdit);

    layout->addStretch();

    connect(m_lineEdit, SIGNAL(textChanged(QString)),
            SLOT(slotEditTextChanged(QString)));
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotDefaultName()));

    setMainWidget(frame);
    slotEditTextChanged(value);
    setMinimumWidth(350);

}

RenameFavoriteDialog::~RenameFavoriteDialog()
{
}

void RenameFavoriteDialog::slotDefaultName()
{
    m_lineEdit->setText(m_defaultName);
}

void RenameFavoriteDialog::slotEditTextChanged(const QString &text)
{
    enableButton(Ok, !text.trimmed().isEmpty());
}

QString RenameFavoriteDialog::newName() const
{
    return m_lineEdit->text();
}
