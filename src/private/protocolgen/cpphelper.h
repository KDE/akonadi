/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class PropertyNode;

#include <QList>

namespace CppHelper
{
void sortMembers(QList<PropertyNode const *> &props);

void sortMembersForSerialization(QList<PropertyNode const *> &props);

} // namespace CppHelper
