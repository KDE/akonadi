/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <QBuffer>
#include <QObject>

#include "private/datastream_p_p.h"
#include "private/protocol_p.h"

#include <type_traits>

class ProtocolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
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
    typename std::enable_if<std::is_base_of<Akonadi::Protocol::Command, T>::value, QSharedPointer<T>>::type serializeAndDeserialize(const QSharedPointer<T> &in)
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);
        Akonadi::Protocol::DataStream stream(&buf);

        Akonadi::Protocol::serialize(stream, in);
        stream.flush();
        buf.seek(0);
        return Akonadi::Protocol::deserialize(&buf).staticCast<T>();
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<Akonadi::Protocol::Command, T>::value == false, T>::type serializeAndDeserialize(const T &in, int * = nullptr)
    {
        QBuffer buf;
        buf.open(QIODevice::ReadWrite);

        Akonadi::Protocol::DataStream stream(&buf);
        stream << in;
        stream.flush();
        buf.seek(0);
        T out;
        stream >> out;

        return out;
    }
};
