/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

class PropertyNode;

#include <QVector>

namespace CppHelper
{
void sortMembers(QVector<PropertyNode const *> &props);

void sortMembersForSerialization(QVector<PropertyNode const *> &props);

} // namespace CppHelper

