/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AGENTINSTANCEWIDGETTEST_H
#define AGENTINSTANCEWIDGETTEST_H

#include <QDialog>

#include "agentinstancewidget.h"

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);

    void done(int) override;

private Q_SLOTS:
    void currentChanged(const Akonadi::AgentInstance &current, const Akonadi::AgentInstance &previous);

private:
    Akonadi::AgentInstanceWidget *mWidget;
};

#endif
