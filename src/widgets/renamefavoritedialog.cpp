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

#include "renamefavoritedialog_p.h"

#include <KLocalizedString>

#include <QPushButton>

using namespace Akonadi;

RenameFavoriteDialog::RenameFavoriteDialog(const QString &value, const QString &defaultName, QWidget *parent)
    : QDialog(parent)
    , m_defaultName(defaultName)
{
    ui.setupUi(this);

    connect(ui.lineEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
                ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
            });
    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &RenameFavoriteDialog::accept);
    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &RenameFavoriteDialog::reject);
    connect(ui.buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, [this]() { ui.lineEdit->setText(m_defaultName); });

    ui.lineEdit->setText(value);
}

QString RenameFavoriteDialog::newName() const
{
    return ui.lineEdit->text();
}
