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

#ifndef AKONADI_AGENTCONFIGURATIONDIALOG
#define AKONADI_AGENTCONFIGURATIONDIALOG

#include <QDialog>

#include "akonadiwidgets_export.h"

namespace Akonadi {

class AgentInstance;
class AKONADIWIDGETS_EXPORT AgentConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AgentConfigurationDialog(const AgentInstance &instance, QWidget *parent = nullptr);
    ~AgentConfigurationDialog() override;

    void accept() override;

private:
    class Private;
    const QScopedPointer<Private> d;
};

}

#endif // AKONADI_AGENTCONFIGURATIONDIALOG
