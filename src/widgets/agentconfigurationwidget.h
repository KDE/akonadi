/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_AGENTCONFIGURATIOWIDGET_H
#define AKONADI_AGENTCONFIGURATIOWIDGET_H

#include <QWidget>
#include "akonadiwidgets_export.h"

namespace Akonadi {

class AgentInstance;
class AgentConfigurationDialog;
/**
 * @brief A widget for displaying agent configuration in applications.
 *
 * To implement an agent configuration widget, see AgentConfigurationBase.
 */
class AKONADIWIDGETS_EXPORT AgentConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AgentConfigurationWidget(const Akonadi::AgentInstance &instance, QWidget *parent = nullptr);
    ~AgentConfigurationWidget() override;

    void load();
    void save();

private:
    class Private;
    friend class Private;
    friend class AgentConfigurationDialog;
    QScopedPointer<Private> d;
};

}


#endif
