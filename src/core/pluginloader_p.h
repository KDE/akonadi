/*  -*- c++ -*-
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
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
