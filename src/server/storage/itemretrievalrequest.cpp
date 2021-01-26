/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemretrievalrequest.h"

using namespace Akonadi::Server;

ItemRetrievalRequest::Id ItemRetrievalRequest::lastId{0};

ItemRetrievalRequest::ItemRetrievalRequest()
    : id(lastId.next())
{
}
