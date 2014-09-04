/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_AGENTTYPE_H
#define AKONADI_AGENTTYPE_H

#include "akonadicore_export.h"

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

class QIcon;
class QString;
class QStringList;
class QVariant;
typedef QMap<QString, QVariant> QVariantMap;

namespace Akonadi {

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
 * foreach ( const Akonadi::AgentType &type, types ) {
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
    typedef QList<AgentType> List;

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
    bool isValid() const;

    /**
     * Returns the unique identifier of the agent type.
     */
    QString identifier() const;

    /**
     * Returns the i18n'ed name of the agent type.
     */
    QString name() const;

    /**
     * Returns the description of the agent type.
     */
    QString description() const;

    /**
     * Returns the name of the icon of the agent type.
     */
    QString iconName() const;

    /**
     * Returns the icon of the agent type.
     */
    QIcon icon() const;

    /**
     * Returns the list of supported mime types of the agent type.
     */
    QStringList mimeTypes() const;

    /**
     * Returns the list of supported capabilities of the agent type.
     */
    QStringList capabilities() const;

    /**
     * Returns a Map of custom properties of the agent type.
     * @since 4.12
     */
    QVariantMap customProperties() const;

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
    //@cond PRIVATE
    class Private;
    QSharedDataPointer<Private> d;
    //@endcond
};

}

Q_DECLARE_TYPEINFO(Akonadi::AgentType, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(Akonadi::AgentType)

#endif
