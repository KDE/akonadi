/*
 * Copyright 2015  Daniel Vratil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for mordetails.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PROTOCOLTEST_H
#define PROTOCOLTEST_H

#include <QtCore/QObject>
#include <QtCore/QBuffer>

#include <private/protocol_p.h>
#include <private/datastream_p_p.h>

#include <type_traits>

class ProtocolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProtocolVersion();

    void testFactory_data();
    void testFactory();

    void testCommand();
    void testResponse_data();
    void testResponse();
    void testAncestor();
    void testFetchScope_data();
    void testFetchScope();
    void testScopeContext_data();
    void testScopeContext();
private:
    template<typename T>
    typename std::enable_if<std::is_base_of<Akonadi::Protocol::Command, T>::value, T>::type
    serializeAndDeserialize(const T &in)
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);

        Akonadi::Protocol::serialize(&buf, in);
        buf.seek(0);
        return T(Akonadi::Protocol::deserialize(&buf));

    }

    template<typename T>
    typename std::enable_if<std::is_base_of<Akonadi::Protocol::Command, T>::value == false, T>::type
    serializeAndDeserialize(const T &in, int * = 0)
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);

        Akonadi::Protocol::DataStream stream(&buf);
        stream << in;
        buf.seek(0);
        T out;
        stream >> out;

        return out;
    }
};

#endif // PROTOCOLTEST_H
