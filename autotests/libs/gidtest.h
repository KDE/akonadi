/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class GidTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSetAndFetch_data();
    void testSetAndFetch();
    void testCreate();
    void testSetWithIgnorePayload();
    void testFetchScope();
};

#include "gidextractorinterface.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"

class TestSerializer : public QObject, public Akonadi::ItemSerializerPlugin, public Akonadi::GidExtractorInterface
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin)
    Q_INTERFACES(Akonadi::GidExtractorInterface)

public:
    bool deserialize(Akonadi::Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Akonadi::Item &item, const QByteArray &label, QIODevice &data, int &version) override;
    QString extractGid(const Akonadi::Item &item) const override;
};
