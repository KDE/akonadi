/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QMetaType>
#include <QSharedDataPointer>

class QIcon;
class QString;
#include <QStringList>
class QVariant;
using QVariantMap = QMap<QString, QVariant>;

namespace Akonadi
{
class AgentTypePrivate;

/**
 * @short A representation of an agent type.
 *
 * The agent type is a representation of an available agent, that can
 * be started as an agent instance.
 * It provides all information about the type.
 *
 * All available agent types can be retrieved from the AgentManager.
 *
 * @code
 *
 * Akonadi::AgentType::List types = Akonadi::AgentManager::self()->types();
 * for ( const Akonadi::AgentType &type : types ) {
 *   qDebug() << "Name:" << type.name() << "(" << type.identifier() << ")";
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT AgentType
{
    friend class AgentManager;
    friend class AgentManagerPrivate;

public:
    /**
     * Describes a list of agent types.
     */
    using List = QList<AgentType>;

    /**
     * Creates a new agent type.
     */
    AgentType();

    /**
     * Creates an agent type from an @p other agent type.
     */
    AgentType(const AgentType &other);

    /**
     * Destroys the agent type.
     */
    ~AgentType();

    /**
     * Returns whether the agent type is valid.
     */
    Q_REQUIRED_RESULT bool isValid() const;

    /**
     * Returns the unique identifier of the agent type.
     */
    Q_REQUIRED_RESULT QString identifier() const;

    /**
     * Returns the i18n'ed name of the agent type.
     */
    Q_REQUIRED_RESULT QString name() const;

    /**
     * Returns the description of the agent type.
     */
    Q_REQUIRED_RESULT QString description() const;

    /**
     * Returns the name of the icon of the agent type.
     */
    Q_REQUIRED_RESULT QString iconName() const;

    /**
     * Returns the icon of the agent type.
     */
    Q_REQUIRED_RESULT QIcon icon() const;

    /**
     * Returns the list of supported mime types of the agent type.
     */
    Q_REQUIRED_RESULT QStringList mimeTypes() const;

    /**
     * Returns the list of supported capabilities of the agent type.
     */
    Q_REQUIRED_RESULT QStringList capabilities() const;

    /**
     * Returns a Map of custom properties of the agent type.
     * @since 4.12
     */
    Q_REQUIRED_RESULT QVariantMap customProperties() const;

    /**
     * @internal
     * @param other other agent type
     */
    AgentType &operator=(const AgentType &other);

    /**
     * @internal
     * @param other other agent type
     */
    bool operator==(const AgentType &other) const;

private:
    /// @cond PRIVATE
    QSharedDataPointer<AgentTypePrivate> d;
    /// @endcond
};

}

Q_DECLARE_TYPEINFO(Akonadi::AgentType, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::AgentType)
