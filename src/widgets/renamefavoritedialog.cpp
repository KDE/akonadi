/*
    Copyright (C) 2011-2020 Laurent Montel <montel@kde.org>

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
#include <QLineEdit>
#include <KLocalizedString>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

RenameFavoriteDialog::RenameFavoriteDialog(const QString &caption, const QString &text, const QString &value, const QString &defaultName, QWidget *parent)
    : QDialog(parent)
    , m_defaultName(defaultName)
{
    setWindowTitle(caption);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_label = new QLabel(text, this);
    m_label->setWordWrap(true);
    mainLayout->addWidget(m_label);

    m_lineEdit = new QLineEdit(value, this);
    m_lineEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_lineEdit);

    m_lineEdit->setFocus();
    m_label->setBuddy(m_lineEdit);

    mainLayout->addStretch();

    connect(m_lineEdit, &QLineEdit::textChanged, this, &RenameFavoriteDialog::slotEditTextChanged);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    QPushButton *defaultButtonName = new QPushButton(i18n("Default Name"), this);
    buttonBox->addButton(defaultButtonName, QDialogButtonBox::ActionRole);
    connect(defaultButtonName, &QPushButton::clicked, this, &RenameFavoriteDialog::slotDefaultName);

    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RenameFavoriteDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RenameFavoriteDialog::reject);
    mainLayout->addWidget(buttonBox);

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
    mOkButton->setEnabled(!text.trimmed().isEmpty());
}

QString RenameFavoriteDialog::newName() const
{
    return m_lineEdit->text();
}
