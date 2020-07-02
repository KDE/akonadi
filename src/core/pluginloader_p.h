/*  -*- c++ -*-
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_PLUGINLOADER_P_H
#define AKONADI_PLUGINLOADER_P_H

#include "akonaditests_export.h"

#include <QHash>
#include <QString>
#include <QObject>

class QPluginLoader;

namespace Akonadi
{

class AKONADI_TESTS_EXPORT PluginMetaData
{
public:
    PluginMetaData();
    PluginMetaData(const QString &lib, const QString &name, const QString &comment, const QString &cname);

    QString library;
    QString nameLabel;
    QString descriptionLabel;
    QString className;
    bool loaded;
};

class AKONADI_TESTS_EXPORT PluginLoader
{
public:
    ~PluginLoader();

    static PluginLoader *self();

    QStringList names() const;

    QObject *createForName(const QString &name);

    PluginMetaData infoForName(const QString &name) const;

    void scan();

private:
    Q_DISABLE_COPY(PluginLoader)
    PluginLoader();

    static PluginLoader *mSelf;
    QHash<QString, QPluginLoader *> mPluginLoaders;
    QHash<QString, PluginMetaData> mPluginInfos;
};

}

#endif
