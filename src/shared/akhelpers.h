/*
    Copyright (C) 2018 - 2019  Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AKHELPERS_H_
#define AKONADI_AKHELPERS_H_

namespace Akonadi
{

static const auto IsNull = [](auto ptr) { return !(bool)ptr; };
static const auto IsNotNull = [](auto ptr) { return (bool)ptr; };


} // namespace Akonadi

#endif
