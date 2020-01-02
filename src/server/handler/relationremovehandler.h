/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_RELATIONREMOVEHANDLER_H_
#define AKONADI_RELATIONREMOVEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

class RelationRemoveHandler: public Handler
{
public:
    RelationRemoveHandler(AkonadiServer &akonadi);
    ~RelationRemoveHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_RELATIONREMOVEHANDLER_H_
