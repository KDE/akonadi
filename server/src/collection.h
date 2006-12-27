/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#ifndef AKONADICOLLECTION_H
#define AKONADICOLLECTION_H

#include <QtCore/QList>
#include <QtCore/QString>

#include "akonadiprivate_export.h"
#include "global.h"

namespace Akonadi {

/**
  @brief A collection of objects.
 */
class AKONADIPRIVATE_EXPORT Collection {
public:
    Collection( const QString & identifier );

    ~Collection();

    void setNoSelect( bool );
    void setNoInferiors( bool );
    void setMimeTypes( const QByteArray & );


    QString identifier() const;
    bool isNoSelect() const;
    bool isNoInferiors() const;
    /** Returns a comman separated list of allowed mime types for this folder */
    QByteArray getMimeTypes() const;

private:
    QString m_identifier;
    bool m_noSelect;
    bool m_noInferiors;
    QByteArray m_mimeTypes;
};

class AKONADIPRIVATE_EXPORT CollectionList : public QList<Collection>
{
  public:
    CollectionList();
  
    void setValid( bool valid );
    bool isValid() const;
  
  private:
    bool mValid;
};

typedef QListIterator<Collection> CollectionListIterator;

}

#endif
