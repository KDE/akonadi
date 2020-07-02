/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#ifndef GLOBAL_H
#define GLOBAL_H

namespace Akonadi
{
namespace Server
{

// rfc1730 section 3
/** The state of the client
*/
enum ConnectionState {
    NonAuthenticated, ///< Not yet authenticated
    Authenticated, ///< The client is authenticated
    LoggingOut
};

} // namespace Server
} // namespace Akonadi

#endif
