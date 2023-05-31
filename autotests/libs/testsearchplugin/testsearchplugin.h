/*
 SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "../../../src/server/search/abstractsearchplugin.h"
#include <QStringList>
#include <searchquery.h>

class TestSearchPlugin : public QObject, public Akonadi::AbstractSearchPlugin
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::AbstractSearchPlugin)
    Q_PLUGIN_METADATA(IID "org.kde.akonadi.TestSearchPlugin" FILE "akonadi_test_searchplugin.json")
public:
    QSet<qint64> search(const QString &query, const QList<qint64> &collections, const QStringList &mimeTypes) override;

    static QSet<qint64> parseQuery(const QString &queryString);
};
