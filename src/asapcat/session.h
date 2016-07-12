/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
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

#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QLocalSocket>
#include <QTime>

class QIODevice;
class QSocketNotifier;

/** ASAP CLI session. */
class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(const QString &input, QObject *parent = Q_NULLPTR);
    ~Session();

    void printStats() const;

public Q_SLOTS:
    void connectToHost();

Q_SIGNALS:
    void disconnected();

private Q_SLOTS:
    void inputAvailable();
    void serverDisconnected();
    void serverError(QLocalSocket::LocalSocketError socketError);
    void serverRead();

private:
    QIODevice *m_input;
    QIODevice *m_session;
    QSocketNotifier *m_notifier;

    QTime m_connectionTime;
    qint64 m_receivedBytes;
    qint64 m_sentBytes;
};

#endif // SESSION_H
