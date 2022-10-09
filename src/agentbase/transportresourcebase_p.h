/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "transportresourcebase.h"

#include <QObject>

class Akonadi__TransportAdaptor;

namespace Akonadi
{
class TransportResourceBase;

/**
  @internal
  This class hosts the D-Bus adaptor for TransportResourceBase.
*/
class TransportResourceBasePrivate : public QObject
{
    Q_OBJECT
public:
    explicit TransportResourceBasePrivate(TransportResourceBase *qq);

Q_SIGNALS:
    /**
     * Emitted when an item has been sent.
     * @param item The id of the item that was sent.
     * @param result The result of the sending operation.
     * @param message An optional textual explanation of the result.
     * @since 4.4
     */
    void transportResult(qlonglong item, int result, const QString &message); // D-Bus signal

private Q_SLOTS:
    void fetchResult(KJob *job);

private:
    friend class TransportResourceBase;
    friend class ::Akonadi__TransportAdaptor;

    void send(Akonadi::Item::Id message); // D-Bus call

    TransportResourceBase *const q;
};

} // namespace Akonadi
