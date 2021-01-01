/*
    SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/widgets/conflictresolvedialog_p.h"

#include <QApplication>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <KAboutData>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("conflictresolvedialogtest_gui"),
                         QStringLiteral("conflictresolvedialogtest_gui"),
                         QStringLiteral("0.10"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QStandardPaths::setTestModeEnabled(true);
    Akonadi::ConflictResolveDialog dlg;
    dlg.exec();

    return 0;
}
