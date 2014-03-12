/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COMMANDBASE_H
#define COMMANDBASE_H

#include <QtCore/QObject>

class CommandBase : public QObject
{
    Q_OBJECT

public:
    explicit CommandBase(QObject *parent = 0);
    virtual void execute() = 0;

    enum Result {
        Undefined,
        OK,
        Canceled,
        Failed
    };

Q_SIGNALS:
    void result(Result);

protected Q_SLOTS:
    virtual void emitResult(Result result);
};

#endif // COMMANDBASE_H
