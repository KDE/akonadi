/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstancewidgettest.h"

#include "agentinstance.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCommandLineParser>
#include <KAboutData>

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    auto *layout = new QVBoxLayout(this);

    mWidget = new Akonadi::AgentInstanceWidget(this);
    connect(mWidget, &Akonadi::AgentInstanceWidget::currentChanged,
            this, &Dialog::currentChanged);

    auto *box = new QDialogButtonBox(this);

    layout->addWidget(mWidget);
    layout->addWidget(box);

    QPushButton *ok = box->addButton(QDialogButtonBox::Ok);
    connect(ok, &QPushButton::clicked, this, &Dialog::accept);

    resize(450, 320);
}

void Dialog::done(int r)
{
    if (r == Accepted) {
        qDebug("'%s' selected", qPrintable(mWidget->currentAgentInstance().identifier()));
    }

    QDialog::done(r);
}

void Dialog::currentChanged(const Akonadi::AgentInstance &current, const Akonadi::AgentInstance &previous)
{
    qDebug("current changed: %s -> %s", qPrintable(previous.identifier()), qPrintable(current.identifier()));
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("agentinstanceviewtest"),
                         QStringLiteral("agentinstanceviewtest"),
                         QStringLiteral("0.10"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Dialog dlg;
    dlg.exec();

    return 0;
}
