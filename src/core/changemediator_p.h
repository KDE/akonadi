/*
    SPDX-FileCopyrightText: 2011 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QObject>

namespace Akonadi
{
class Job;
class JobPrivate;

class Collection;
class Item;
class Tag;

class ChangeMediator : public QObject
{
    Q_OBJECT
public:
    static ChangeMediator *instance();

    static void registerMonitor(QObject *monitor);
    static void unregisterMonitor(QObject *monitor);

    static void invalidateCollection(const Akonadi::Collection &collection);
    static void invalidateItem(const Akonadi::Item &item);
    static void invalidateTag(const Akonadi::Tag &tag);

protected:
    explicit ChangeMediator(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(ChangeMediator)

    QList<QObject *> m_monitors;
};

}
