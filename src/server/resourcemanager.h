/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RESOURCEMANAGER_H
#define AKONADI_RESOURCEMANAGER_H

#include <QObject>

namespace Akonadi
{
namespace Server
{
class Tracer;

/**
  Listens to agent instance added/removed signals and creates/removes
  the corresponding data in the database.
*/
class ResourceManager : public QObject
{
    Q_OBJECT

public:
    explicit ResourceManager(Tracer &tracer);

    QStringList resourceInstances() const;

public Q_SLOTS:
    void addResourceInstance(const QString &name, const QStringList &capabilities);
    void removeResourceInstance(const QString &name);

private:
    Tracer &mTracer;
};

} // namespace Server
} // namespace Akonadi

#endif
