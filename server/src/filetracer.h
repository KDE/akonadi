/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#ifndef AKONADI_FILETRACER_H
#define AKONADI_FILETRACER_H

#include "tracerinterface.h"

class QFile;

namespace Akonadi {

/**
 * A tracer which forwards all tracing information to a
 * log file.
 */
class FileTracer : public TracerInterface
{
  public:
    FileTracer( const QString &fileName );
    virtual ~FileTracer();

    virtual void beginConnection( const QString&, const QString& );
    virtual void endConnection( const QString&, const QString& );
    virtual void connectionInput( const QString&, const QString& );
    virtual void connectionOutput( const QString&, const QString& );
    virtual void signal( const QString&, const QString& );
    virtual void warning( const QString&, const QString& );
    virtual void error( const QString&, const QString& );

  private:
    void output( const QString&, const QString& );

    QFile *m_file;
};

}

#endif
