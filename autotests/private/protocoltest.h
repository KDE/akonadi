/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef PROTOCOLTEST_H
#define PROTOCOLTEST_H

#include <QObject>
#include <QBuffer>

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
    void testPartMetaData();
    void testCachePolicy();

    void testHelloResponse();
    void testLoginCommand();
    void testLoginResponse();
    void testLogoutCommand();
    void testLogoutResponse();
    void testTransactionCommand();
    void testTransactionResponse();
    void testCreateItemCommand();
    void testCreateItemResponse();
    void testCopyItemsCommand();
    void testCopyItemsResponse();

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
