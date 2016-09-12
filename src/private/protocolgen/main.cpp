/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.og>

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

#include <QCoreApplication>
#include <QCommandLineParser>

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
