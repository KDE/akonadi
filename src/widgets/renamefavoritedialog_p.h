/*
    SPDX-FileCopyrightText: 2011-2020 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RENAMEFAVORITEDIALOG_H
#define RENAMEFAVORITEDIALOG_H

#include <QDialog>

#include "ui_renamefavoritedialog.h"

namespace Akonadi
{

class RenameFavoriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RenameFavoriteDialog(const QString &value, const QString &defaultName, QWidget *parent);

    Q_REQUIRED_RESULT QString newName() const;

private:
    QString m_defaultName;
    Ui::RenameFavoriteDialog ui;
};

}

#endif /* RENAMEFAVORITEDIALOG_H */
