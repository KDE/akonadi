/***************************************************************************
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <QElapsedTimer>
#include <QLocalSocket>
#include <QObject>

class QIODevice;
class QSocketNotifier;

/** ASAP CLI session. */
class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(const QString &input, QObject *parent = nullptr);
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
    QIODevice *m_input = nullptr;
    QIODevice *m_session = nullptr;
    QSocketNotifier *m_notifier = nullptr;

    QElapsedTimer m_connectionTime;
    qint64 m_receivedBytes = 0;
    qint64 m_sentBytes = 0;
};

#endif // SESSION_H
