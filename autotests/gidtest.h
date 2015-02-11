/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef GIDTEST_H
#define GIDTEST_H

#include <QtCore/QObject>

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

#include <../src/core/gidextractorinterface.h>
#include <itemserializer_p.h>
#include <itemserializerplugin.h>

class TestSerializer : public QObject,
                       public Akonadi::ItemSerializerPlugin,
                       public Akonadi::GidExtractorInterface
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin)
    Q_INTERFACES(Akonadi::GidExtractorInterface)

public:
    bool deserialize(Akonadi::Item &item, const QByteArray &label, QIODevice &data, int version) Q_DECL_OVERRIDE;
    void serialize(const Akonadi::Item &item, const QByteArray &label, QIODevice &data, int &version) Q_DECL_OVERRIDE;
    QString extractGid(const Akonadi::Item &item) const Q_DECL_OVERRIDE;
};

#endif
