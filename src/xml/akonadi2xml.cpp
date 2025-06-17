/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xmlwritejob.h"

#include "collection.h"
#include "collectionpathresolver.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

using namespace Akonadi;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("akonadi2xml"),
                         i18n("Akonadi To XML converter"),
                         QStringLiteral("1.0"),
                         i18n("Converts an Akonadi collection subtree into a XML file."),
                         KAboutLicense::GPL,
                         i18n("(c) 2009 Volker Krause <vkrause@kde.org>"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Collection root;
    if (parser.isSet(QStringLiteral("collection"))) {
        const QString path = parser.value(QStringLiteral("collection"));
        CollectionPathResolver resolver(path);
        if (!resolver.exec()) {
            qCritical() << resolver.errorString();
            return -1;
        }
        root = Collection(resolver.collection());
    } else {
        return -1;
    }

    XmlWriteJob writer(root, parser.value(QStringLiteral("output")));
    if (!writer.exec()) {
        qCritical() << writer.exec();
        return -1;
    }
}
