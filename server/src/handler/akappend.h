/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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
#ifndef AKONADI_AKAPPEND_H
#define AKONADI_AKAPPEND_H

#include <handler.h>
#include <entities.h>

#include <QtCore/QDateTime>
#include <QtCore/QVector>

class QTemporaryFile;
namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the X-AKAPPEND command.

  This command is used to append an item with multiple parts.

 */
class AkAppend : public Handler
{
    Q_OBJECT
public:
    AkAppend();

    virtual ~AkAppend();

    virtual bool parseStream();

protected:
    class ChangedAttributes
    {
    public:
        ChangedAttributes()
            : incremental(false)
        {
        }

        bool incremental;
        QVector<QByteArray> added;
        QVector<QByteArray> removed;
    };

    bool buildPimItem(PimItem &item,
                      Collection &parentCollection,
                      ChangedAttributes &flags,
                      ChangedAttributes &tagsRID,
                      ChangedAttributes &tagsGID);

    bool insertItem(PimItem &item,
                    const Collection &parentCollection,
                    const QVector<QByteArray> &itemFlags,
                    const QVector<QByteArray> &itemTagsRID,
                    const QVector<QByteArray> &itemTagsGID);

    bool readParts(PimItem &item);

    virtual bool notify(const PimItem &item, const Collection &collection);
    virtual bool sendResponse(const QByteArray &response, const PimItem &item);

private:
    QByteArray parseFlag(const QByteArray &flag) const;

};

} // namespace Server
} // namespace Akonadi

#endif
