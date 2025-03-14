/*
    SPDX-FileCopyrightText: 2011-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "ui_renamefavoritedialog.h"

namespace Akonadi
{
class RenameFavoriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RenameFavoriteDialog(const QString &value, const QString &defaultName, QWidget *parent);

    [[nodiscard]] QString newName() const;

private:
    const QString m_defaultName;
    Ui::RenameFavoriteDialog ui;
};

}
