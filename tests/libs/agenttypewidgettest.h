/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "agenttypewidget.h"

class QComboBox;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);

    void done(int) override;

private Q_SLOTS:
    void currentChanged(const Akonadi::AgentType &current, const Akonadi::AgentType &previous);
    void filterChanged(int);

private:
    Akonadi::AgentTypeWidget *mWidget;
    QComboBox *mFilter;
};
