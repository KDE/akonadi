/*
    SPDX-FileCopyrightText: 2011-2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

    connect(ui.lineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
    });
    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &RenameFavoriteDialog::accept);
    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &RenameFavoriteDialog::reject);
    connect(ui.buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, [this]() {
        ui.lineEdit->setText(m_defaultName);
    });

    ui.lineEdit->setText(value);
}

QString RenameFavoriteDialog::newName() const
{
    return ui.lineEdit->text();
}
