/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SESSION_P_H
#define AKONADI_SESSION_P_H

#include "session.h"
#include "imapparser.h"
#include <QQueue>
#include <QTcpSocket>
#include <QThreadStorage>


namespace Akonadi {

class Session::Private
{
  public:
    Private( Session *parent )
      : mParent( parent )
    {
    }

    void startNext();
    void reconnect();
    void socketError();
    void dataReceived();
    void doStartNext();
    void jobDone( KJob* job );

    Session *mParent;
    QByteArray sessionId;
    QTcpSocket* socket;
    bool connected;
    int nextTag;

    // job management
    QQueue<Job*> queue;
    Job* currentJob;
    bool jobRunning;

    // parser stuff
    QByteArray dataBuffer;
    QByteArray tagBuffer;
    int parenthesesCount;
    int literalSize;

    void clearParserState()
    {
      dataBuffer.clear();
      tagBuffer.clear();
      parenthesesCount = 0;
      literalSize = 0;
    }

    // Returns true iff response is complete, false otherwise.
    bool parseNextLine()
    {
      QByteArray readBuffer = socket->readLine();

      // first line, get the tag
      if ( dataBuffer.isEmpty() ) {
        int startOfData = readBuffer.indexOf( ' ' );
        tagBuffer = readBuffer.left( startOfData );
        dataBuffer = readBuffer.mid( startOfData + 1 );
      } else {
        dataBuffer += readBuffer;
      }

      // literal read in progress
      if ( literalSize > 0 ) {
        literalSize -= readBuffer.size();

        // still not everything read
        if ( literalSize > 0 )
          return false;

        // check the remaining (non-literal) part for parentheses
        if ( literalSize < 0 )
          // the following looks strange but works since literalSize can be negative here
          parenthesesCount = ImapParser::parenthesesBalance( readBuffer, readBuffer.length() + literalSize );

        // literal string finished but still open parentheses
        if ( parenthesesCount > 0 )
            return false;

      } else {

        // open parentheses
        parenthesesCount += ImapParser::parenthesesBalance( readBuffer );

        // start new literal read
        if ( readBuffer.trimmed().endsWith( '}' ) ) {
          int begin = readBuffer.lastIndexOf( '{' );
          int end = readBuffer.lastIndexOf( '}' );
          literalSize = readBuffer.mid( begin + 1, end - begin - 1 ).toInt();
          return false;
        }

        // still open parentheses
        if ( parenthesesCount > 0 )
          return false;

        // just a normal response, fall through
      }

      return true;
    }
};

}

#endif
