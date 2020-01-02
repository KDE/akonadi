/***************************************************************************
 *   Copyright (C) 2009 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AKONADI_SEARCHHANDLER_H_ 
#define AKONADI_SEARCHHANDLER_H_

#include "handler.h"

#include <QSet>

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the search commands.
*/
class SearchHandler : public Handler
{
public:
    SearchHandler(AkonadiServer &akonadi);
    ~SearchHandler() override = default;

    bool parseStream() override;

private:
    void processResults(const QSet<qint64> &results);

    Protocol::ItemFetchScope mItemFetchScope;
    Protocol::TagFetchScope mTagFetchScope;
    QSet<qint64> mAllResults;
};

} // namespace Server
} // namespace Akonadi

#endif
