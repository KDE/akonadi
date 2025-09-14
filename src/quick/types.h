// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <qqmlintegration.h>

#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstance>
#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Item>
#include <Akonadi/SpecialCollections>

#define FOREIGN_ENUM_GADGET(Class, ValueType)                                                                                                                  \
    struct Class##Foreign {                                                                                                                                    \
        Q_GADGET                                                                                                                                               \
        QML_FOREIGN(Akonadi::Class)                                                                                                                            \
        QML_VALUE_TYPE(ValueType)                                                                                                                              \
    };                                                                                                                                                         \
    struct Class##Derived : public Akonadi::Class {                                                                                                            \
        Q_GADGET                                                                                                                                               \
    };                                                                                                                                                         \
    namespace Class##DerivedForeign                                                                                                                            \
    {                                                                                                                                                          \
        Q_NAMESPACE                                                                                                                                            \
        QML_NAMED_ELEMENT(Class)                                                                                                                               \
        QML_FOREIGN_NAMESPACE(Class##Derived)                                                                                                                  \
    }

FOREIGN_ENUM_GADGET(Collection, collection)
FOREIGN_ENUM_GADGET(Item, item)

class EntityTreeModelForeign : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(EntityTreeModel)
    QML_FOREIGN(Akonadi::EntityTreeModel)
    QML_UNCREATABLE("Only enums")
};

class AgentInstanceForeign
{
    Q_GADGET
    QML_VALUE_TYPE(agentInstance)
    QML_FOREIGN(Akonadi::AgentInstance)
};

class SpecialCollections : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SpecialCollections)
    QML_FOREIGN(Akonadi::SpecialCollections)
    QML_UNCREATABLE("Abstract class")
};

class AgentFilterProxyModelForeign : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AgentFilterProxyModel)
    QML_FOREIGN(Akonadi::AgentFilterProxyModel)
    QML_UNCREATABLE("C++ only")
};
