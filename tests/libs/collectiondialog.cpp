/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiondialog.h"

#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>

using namespace Akonadi;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("test"), i18n("Test Application"), QStringLiteral("1.0"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    CollectionDialog dlg;
    dlg.setMimeTypeFilter({QStringLiteral("text/directory")});
    dlg.setAccessRightsFilter(Collection::CanCreateItem);
    dlg.setDescription(i18n("Select an address book for saving:"));
    dlg.setSelectionMode(QAbstractItemView::ExtendedSelection);
    dlg.changeCollectionDialogOptions(CollectionDialog::AllowToCreateNewChildCollection);
    dlg.exec();

    foreach (const Collection &collection, dlg.selectedCollections()) {
        qDebug() << "Selected collection:" << collection.name();
    }

    return 0;
}
