/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_SERVER_CONNECTIONTHREAD_H
#define AKONADI_SERVER_CONNECTIONTHREAD_H

#include <QThread>

namespace Akonadi {
namespace Server {

class ConnectionThread : public QThread
{
    Q_OBJECT

public:
    explicit ConnectionThread(quintptr socketDescriptor, QObject *parent = 0);
    virtual ~ConnectionThread();

    void run();

private:
    quintptr mSocketDescriptor;
};

}
}

#endif // AKONADI_SERVER_CONNECTIONTHREAD_H
