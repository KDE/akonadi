/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agenttypewidgettest.h"

#include "agentfilterproxymodel.h"
#include "agenttype.h"

#include <KAboutData>
#include <QComboBox>

#include <QApplication>
#include <QCommandLineParser>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

// krazy:excludeall=qclasses

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    auto layout = new QVBoxLayout(this);

    mFilter = new QComboBox(this);
    mFilter->addItem(QStringLiteral("None"));
    mFilter->addItem(QStringLiteral("text/calendar"));
    mFilter->addItem(QStringLiteral("text/directory"));
    mFilter->addItem(QStringLiteral("message/rfc822"));
    connect(mFilter, &QComboBox::activated, this, &Dialog::filterChanged);

    mWidget = new Akonadi::AgentTypeWidget(this);
    connect(mWidget, &Akonadi::AgentTypeWidget::currentChanged, this, &Dialog::currentChanged);

    auto box = new QDialogButtonBox(this);

    layout->addWidget(mFilter);
    layout->addWidget(mWidget);
    layout->addWidget(box);

    QPushButton *ok = box->addButton(QDialogButtonBox::Ok);
    connect(ok, &QPushButton::clicked, this, &Dialog::accept);

    QPushButton *cancel = box->addButton(QDialogButtonBox::Cancel);
    connect(cancel, &QPushButton::clicked, this, &Dialog::reject);

    resize(450, 320);
}

void Dialog::done(int r)
{
    if (r == Accepted) {
        qDebug("'%s' selected", qPrintable(mWidget->currentAgentType().identifier()));
    }

    QDialog::done(r);
}

void Dialog::currentChanged(const Akonadi::AgentType &current, const Akonadi::AgentType &previous)
{
    qDebug("current changed: %s -> %s", qPrintable(previous.identifier()), qPrintable(current.identifier()));
}

void Dialog::filterChanged(int index)
{
    mWidget->agentFilterProxyModel()->clearFilters();
    if (index > 0) {
        mWidget->agentFilterProxyModel()->addMimeTypeFilter(mFilter->itemText(index));
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("agenttypeviewtest"), QStringLiteral("agenttypeviewtest"), QStringLiteral("0.10"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Dialog dlg;
    dlg.exec();

    return 0;
}

#include "moc_agenttypewidgettest.cpp"
