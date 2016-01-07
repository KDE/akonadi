/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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
    QVBoxLayout *layout = new QVBoxLayout(this);

    mWidget = new Akonadi::AgentInstanceWidget(this);
    connect(mWidget, &Akonadi::AgentInstanceWidget::currentChanged,
            this, &Dialog::currentChanged);

    QDialogButtonBox *box = new QDialogButtonBox(this);

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
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Dialog dlg;
    dlg.exec();

    return 0;
}
