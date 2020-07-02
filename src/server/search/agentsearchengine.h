/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AGENTSEARCHENGINE_H
#define AGENTSEARCHENGINE_H

#include "abstractsearchengine.h"

namespace Akonadi
{
namespace Server
{

/** Search engine for distributing searches to agents. */
class AgentSearchEngine : public AbstractSearchEngine
{
public:
    void addSearch(const Collection &collection) override;
    void removeSearch(qint64 id) override;
};

} // namespace Server
} // namespace Akonadi

#endif
