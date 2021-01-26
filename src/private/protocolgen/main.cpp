/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QCommandLineParser>
#include <QCoreApplication>

#include "cppgenerator.h"
#include "xmlparser.h"

#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("File"));
    parser.addHelpOption();
    parser.process(app);

    const auto args = parser.positionalArguments();
    if (args.isEmpty()) {
        std::cerr << "No file specified" << std::endl;
        return 1;
    }

    XmlParser xmlParser;
    if (!xmlParser.parse(args[0])) {
        return -1;
    }

    CppGenerator cppGenerator;
    if (!cppGenerator.generate(xmlParser.tree())) {
        return -2;
    } else {
        return 0;
    }
}
