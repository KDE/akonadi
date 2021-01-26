/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemserializerplugin.h"
#include "pluginloader_p.h"

#include <QApplication>
#include <QDebug>
#include <QStringList>

using namespace Akonadi;

int main()
{
    QApplication::setApplicationName(QStringLiteral("pluginloadertest"));

    PluginLoader *loader = PluginLoader::self();

    const QStringList types = loader->names();
    qDebug("Types:");
    for (int i = 0; i < types.count(); ++i) {
        qDebug("%s", qPrintable(types.at(i)));
    }

    QObject *object = loader->createForName(QStringLiteral("text/vcard@KContacts::Addressee"));
    if (qobject_cast<ItemSerializerPlugin *>(object) != nullptr) {
        qDebug("Loaded plugin for mimetype 'text/vcard@KContacts::Addressee' successfully");
    } else {
        qDebug("Unable to load plugin for mimetype 'text/vcard'");
    }

    return 0;
}
