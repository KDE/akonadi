/***************************************************************************
 *   SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QObject>

/**
 * The control manager provides a dbus method to shutdown
 * the Akonadi Control process cleanly.
 */
class ControlManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new control manager.
     */
    explicit ControlManager(QObject *parent = nullptr);

    /**
     * Destroys the control manager.
     */
    ~ControlManager();

public Q_SLOTS:
    /**
     * Shutdown the Akonadi Control process cleanly.
     */
    void shutdown();
};

