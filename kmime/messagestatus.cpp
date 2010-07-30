/*
    This file is part of Akonadi.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>
    Copyright (c) 2010 KDAB
    Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "messagestatus.h"

#include <KDE/KDebug>
#include <QString>
#include "messageflags.h"

/** The message status format. These can be or'd together.
    Note, that the StatusIgnored implies the
    status to be Read even if the flags are set
    to Unread. This is done in isRead()
    and related getters. So we can preserve the state
    when switching a thread to Ignored and back. */
enum Status {
    StatusUnknown =           0x00000000,
    StatusUnread =            0x00000002,
    StatusRead =              0x00000004,
    StatusDeleted =           0x00000010,
    StatusReplied =           0x00000020,
    StatusForwarded =         0x00000040,
    StatusQueued =            0x00000080,
    StatusSent =              0x00000100,
    StatusFlag =              0x00000200, // flag means important
    StatusWatched =           0x00000400,
    StatusIgnored =           0x00000800, // forces isRead()
    StatusToAct =             0x00001000,
    StatusSpam =              0x00002000,
    StatusHam =               0x00004000,
    StatusHasAttach =         0x00008000,
    StatusHasInvitation =     0x00010000
};

Akonadi::MessageStatus::MessageStatus()
{
  mStatus = StatusUnknown;
}

Akonadi::MessageStatus &Akonadi::MessageStatus::operator = ( const Akonadi::MessageStatus &other )
{
  mStatus = other.mStatus;
  return *this;
}

bool Akonadi::MessageStatus::operator == ( const Akonadi::MessageStatus &other ) const
{
  return ( mStatus == other.mStatus );
}

bool Akonadi::MessageStatus::operator != ( const Akonadi::MessageStatus &other ) const
{
  return ( mStatus != other.mStatus );
}

bool Akonadi::MessageStatus::operator & ( const Akonadi::MessageStatus &other ) const
{
  return ( mStatus & other.mStatus );
}

void Akonadi::MessageStatus::clear()
{
  mStatus = StatusUnknown;
}

void Akonadi::MessageStatus::set( const Akonadi::MessageStatus &other )
{
  // Those stati are exclusive, but we have to lock at the
  // internal representation because Ignored can manipulate
  // the result of the getter methods.
  if ( other.mStatus & StatusUnread ) {
    setUnread();
  }
  if ( other.mStatus & StatusRead ) {
    setRead();
  }
  if ( other.isDeleted() ) {
    setDeleted();
  }
  if ( other.isReplied() ) {
    setReplied();
  }
  if ( other.isForwarded() ) {
    setForwarded();
  }
  if ( other.isQueued() ) {
    setQueued();
  }
  if ( other.isSent() ) {
    setSent();
  }
  if ( other.isImportant() ) {
    setImportant();
  }

  if ( other.isWatched() ) {
    setWatched();
  }
  if ( other.isIgnored() ) {
    setIgnored();
  }
  if ( other.isToAct() ) {
    setToAct();
  }
  if ( other.isSpam() ) {
    setSpam();
  }
  if ( other.isHam() ) {
    setHam();
  }
  if ( other.hasAttachment() ) {
    setHasAttachment();
  }
  if( other.hasInvitation() ) {
    setHasInvitation();
  }
}

void Akonadi::MessageStatus::toggle( const Akonadi::MessageStatus &other )
{
  if ( other.isDeleted() ) {
    setDeleted( !( mStatus & StatusDeleted ) );
  }
  if ( other.isReplied() ) {
    setReplied( !( mStatus & StatusReplied ) );
  }
  if ( other.isForwarded() ) {
    setForwarded( !( mStatus & StatusForwarded ) );
  }
  if ( other.isQueued() ) {
    setQueued( !( mStatus & StatusQueued ) );
  }
  if ( other.isSent() ) {
    setSent( !( mStatus & StatusSent ) );
  }
  if ( other.isImportant() ) {
    setImportant( !( mStatus & StatusFlag ) );
  }

  if ( other.isWatched() ) {
    setWatched( !( mStatus & StatusWatched ) );
  }
  if ( other.isIgnored() ) {
    setIgnored( !( mStatus & StatusIgnored ) );
  }
  if ( other.isToAct() ) {
    setToAct( !( mStatus & StatusToAct ) );
  }
  if ( other.isSpam() ) {
    setSpam( !( mStatus & StatusSpam ) );
  }
  if ( other.isHam() ) {
    setHam( !( mStatus & StatusHam ) );
  }
  if ( other.hasAttachment() ) {
    setHasAttachment( !( mStatus & StatusHasAttach ) );
  }
  if( other.hasInvitation() ) {
    setHasInvitation( !( mStatus & StatusHasInvitation ) );
  }
}

bool Akonadi::MessageStatus::isOfUnknownStatus() const
{
  return ( mStatus == StatusUnknown );
}

bool Akonadi::MessageStatus::isUnread() const
{
  return ( mStatus & StatusUnread && !( mStatus & StatusIgnored ) );
}

bool Akonadi::MessageStatus::isRead() const
{
  return ( mStatus & StatusRead || mStatus & StatusIgnored );
}

bool Akonadi::MessageStatus::isDeleted() const
{
  return ( mStatus & StatusDeleted );
}

bool Akonadi::MessageStatus::isReplied() const
{
  return ( mStatus & StatusReplied );
}

bool Akonadi::MessageStatus::isForwarded() const
{
  return ( mStatus & StatusForwarded );
}

bool Akonadi::MessageStatus::isQueued() const
{
  return ( mStatus & StatusQueued );
}

bool Akonadi::MessageStatus::isSent() const
{
   return ( mStatus & StatusSent );
}

bool Akonadi::MessageStatus::isImportant() const
{
  return ( mStatus & StatusFlag );
}

bool Akonadi::MessageStatus::isWatched() const
{
  return ( mStatus & StatusWatched );
}

bool Akonadi::MessageStatus::isIgnored() const
{
  return ( mStatus & StatusIgnored );
}

bool Akonadi::MessageStatus::isToAct() const
{
  return ( mStatus & StatusToAct );
}

bool Akonadi::MessageStatus::isSpam() const
{
  return ( mStatus & StatusSpam );
}

bool Akonadi::MessageStatus::isHam() const
{
  return ( mStatus & StatusHam );
}

bool Akonadi::MessageStatus::hasAttachment() const
{
  return ( mStatus & StatusHasAttach );
}

bool Akonadi::MessageStatus::hasInvitation() const
{
  return ( mStatus & StatusHasInvitation );
}


void Akonadi::MessageStatus::setUnread()
{
  // unread overrides read
  mStatus &= ~StatusRead;
  mStatus |= StatusUnread;
}

void Akonadi::MessageStatus::setRead()
{
  // Unset unread and set read
  mStatus &= ~StatusUnread;
  mStatus |= StatusRead;
}

void Akonadi::MessageStatus::setDeleted( bool deleted )
{
  if ( deleted ) {
    mStatus |= StatusDeleted;
  } else {
    mStatus &= ~StatusDeleted;
  }
}

void Akonadi::MessageStatus::setReplied( bool replied )
{
  if ( replied ) {
    mStatus |= StatusReplied;
  } else {
    mStatus &= ~StatusReplied;
  }
}

void Akonadi::MessageStatus::setForwarded( bool forwarded )
{
  if ( forwarded ) {
    mStatus |= StatusForwarded;
  } else {
    mStatus &= ~StatusForwarded;
  }
}

void Akonadi::MessageStatus::setQueued( bool queued )
{
  if ( queued ) {
    mStatus |= StatusQueued;
  } else {
    mStatus &= ~StatusQueued;
  }
}

void Akonadi::MessageStatus::setSent( bool sent )
{
  if ( sent ) {
    mStatus &= ~StatusQueued;
    // FIXME to be discussed if sent messages are Read
    mStatus &= ~StatusUnread;
    mStatus |= StatusSent;
  } else {
    mStatus &= ~StatusSent;
  }
}

void Akonadi::MessageStatus::setImportant( bool important )
{
  if ( important ) {
    mStatus |= StatusFlag;
  } else {
    mStatus &= ~StatusFlag;
  }
}

// Watched and ignored are mutually exclusive
void Akonadi::MessageStatus::setWatched( bool watched )
{
  if ( watched ) {
    mStatus &= ~StatusIgnored;
    mStatus |= StatusWatched;
  } else {
    mStatus &= ~StatusWatched;
  }
}

void Akonadi::MessageStatus::setIgnored( bool ignored )
{
  if ( ignored ) {
    mStatus &= ~StatusWatched;
    mStatus |= StatusIgnored;
  } else {
    mStatus &= ~StatusIgnored;
  }
}

void Akonadi::MessageStatus::setToAct( bool toAct )
{
  if ( toAct ) {
    mStatus |= StatusToAct;
  } else {
    mStatus &= ~StatusToAct;
  }
}

// Ham and Spam are mutually exclusive
void Akonadi::MessageStatus::setSpam( bool spam )
{
  if ( spam ) {
    mStatus &= ~StatusHam;
    mStatus |= StatusSpam;
  } else {
    mStatus &= ~StatusSpam;
  }
}

void Akonadi::MessageStatus::setHam( bool ham )
{
  if ( ham ) {
    mStatus &= ~StatusSpam;
    mStatus |= StatusHam;
  } else {
    mStatus &= ~StatusHam;
  }
}

void Akonadi::MessageStatus::setHasAttachment( bool withAttachment )
{
  if ( withAttachment ) {
    mStatus |= StatusHasAttach;
  } else {
    mStatus &= ~StatusHasAttach;
  }
}

void Akonadi::MessageStatus::setHasInvitation( bool withInvitation )
{
  if ( withInvitation ) {
    mStatus |= StatusHasInvitation;
  } else {
    mStatus &= ~StatusHasInvitation;
  }
}

qint32 Akonadi::MessageStatus::toQInt32() const
{
  return mStatus;
}


void Akonadi::MessageStatus::fromQInt32( qint32 status )
{
  mStatus = status;
}

QString Akonadi::MessageStatus::statusStr() const
{
  QByteArray sstr;
  if ( mStatus & StatusUnread ) {
    sstr += 'U';
  }
  if ( mStatus & StatusRead ) {
    sstr += 'R';
  }
  if ( mStatus & StatusDeleted ) {
    sstr += 'D';
  }
  if ( mStatus & StatusReplied ) {
    sstr += 'A';
  }
  if ( mStatus & StatusForwarded ) {
    sstr += 'F';
  }
  if ( mStatus & StatusQueued ) {
    sstr += 'Q';
  }
  if ( mStatus & StatusToAct ) {
    sstr += 'K';
  }
  if ( mStatus & StatusSent ) {
    sstr += 'S';
  }
  if ( mStatus & StatusFlag ) {
    sstr += 'G';
  }
  if ( mStatus & StatusWatched ) {
    sstr += 'W';
  }
  if ( mStatus & StatusIgnored ) {
    sstr += 'I';
  }
  if ( mStatus & StatusSpam ) {
    sstr += 'P';
  }
  if ( mStatus & StatusHam ) {
    sstr += 'H';
  }
  if ( mStatus & StatusHasAttach ) {
    sstr += 'T';
  }

  return QLatin1String( sstr );
}

void Akonadi::MessageStatus::setStatusFromStr( const QString& aStr )
{
  mStatus = StatusUnknown;

  if ( aStr.contains( QLatin1Char( 'U' ) ) ) {
    setUnread();
  }
  if ( aStr.contains( QLatin1Char( 'R' ) ) ) {
    setRead();
  }
  if ( aStr.contains( QLatin1Char( 'D' ) ) ) {
    setDeleted();
  }
  if ( aStr.contains( QLatin1Char( 'A' ) ) ) {
    setReplied();
  }
  if ( aStr.contains( QLatin1Char( 'F' ) ) ) {
    setForwarded();
  }
  if ( aStr.contains( QLatin1Char( 'Q' ) ) ) {
    setQueued();
  }
  if ( aStr.contains( QLatin1Char( 'K' ) ) ) {
    setToAct();
  }
  if ( aStr.contains( QLatin1Char( 'S' ) ) ) {
    setSent();
  }
  if ( aStr.contains( QLatin1Char( 'G' ) ) ) {
    setImportant();
  }
  if ( aStr.contains( QLatin1Char( 'W' ) ) ) {
    setWatched();
  }
  if ( aStr.contains( QLatin1Char( 'I' ) ) ) {
    setIgnored();
  }
  if ( aStr.contains( QLatin1Char( 'P' ) ) ) {
    setSpam();
  }
  if ( aStr.contains( QLatin1Char( 'H' ) ) ) {
    setHam();
  }
  if ( aStr.contains( QLatin1Char( 'T' ) ) ) {
    setHasAttachment();
  }
  if ( aStr.contains( QLatin1Char( 'C' ) ) ) {
    setHasAttachment( false );
  }
}

QSet<QByteArray> Akonadi::MessageStatus::statusFlags() const
{
  QSet<QByteArray> flags;

  // Non handled status:
  // * StatusQueued
  // * StatusSent
  // * StatusSpam
  // * StatusHam
  // * StatusHasAttach

  if ( mStatus & StatusDeleted ) {
    flags+= Akonadi::MessageFlags::Deleted;
  } else {
    if ( mStatus &  StatusRead )
      flags+= Akonadi::MessageFlags::Seen;
    if ( mStatus & StatusReplied )
      flags+= Akonadi::MessageFlags::Answered;
    if ( mStatus & StatusFlag )
      flags+= Akonadi::MessageFlags::Flagged;
    // non standard flags
    if ( mStatus & StatusForwarded )
      flags+= "$FORWARDED";
    if ( mStatus & StatusToAct )
      flags+= "$TODO";
    if ( mStatus & StatusWatched )
      flags+= "$WATCHED";
    if ( mStatus & StatusIgnored )
      flags+= "$IGNORED";
    if ( mStatus & StatusHasAttach )
      flags+= Akonadi::MessageFlags::Attachment;
  }

  return flags;
}

void Akonadi::MessageStatus::setStatusFromFlags( const QSet<QByteArray> &flags )
{
  mStatus = StatusUnknown;
  setUnread();
  // Non handled status:
  // * StatusQueued
  // * StatusSent
  // * StatusSpam
  // * StatusHam
  // * StatusHasAttach

  foreach ( const QByteArray &flag, flags ) {
    const QByteArray &upperedFlag = flag.toUpper();
    if ( upperedFlag ==  Akonadi::MessageFlags::Deleted ) {
      setDeleted();
    } else if ( upperedFlag ==  Akonadi::MessageFlags::Seen ) {
      setRead();
    } else if ( upperedFlag ==  Akonadi::MessageFlags::Answered ) {
      setReplied();
    } else if ( upperedFlag ==  Akonadi::MessageFlags::Flagged ) {
      setImportant();

    // non standard flags
    } else if ( upperedFlag ==  "$FORWARDED" ) {
      setForwarded();
    } else if ( upperedFlag ==  "$TODO" ) {
      setToAct();
    } else if ( upperedFlag ==  "$WATCHED" ) {
      setWatched();
    } else if ( upperedFlag ==  "$IGNORED" ) {
      setIgnored();
    } else if ( upperedFlag ==  "$JUNK" ) {
      setSpam();
    } else if ( upperedFlag ==  "$NOTJUNK" ) {
      setHam();
    } else if ( upperedFlag ==  Akonadi::MessageFlags::Attachment ) {
      setHasAttachment( true );
    } else {
      kWarning() << "Unknown flag:" << flag;
    }
  }
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusRead()
{
  Akonadi::MessageStatus st;
  st.setRead();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusUnread()
{
  Akonadi::MessageStatus st;
  st.setUnread();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusDeleted()
{
  Akonadi::MessageStatus st;
  st.setDeleted();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusReplied()
{
  Akonadi::MessageStatus st;
  st.setReplied();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusForwarded()
{
  Akonadi::MessageStatus st;
  st.setForwarded();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusQueued()
{
  Akonadi::MessageStatus st;
  st.setQueued();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusSent()
{
  Akonadi::MessageStatus st;
  st.setSent();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusImportant()
{
  Akonadi::MessageStatus st;
  st.setImportant();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusWatched()
{
  Akonadi::MessageStatus st;
  st.setWatched();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusIgnored()
{
  Akonadi::MessageStatus st;
  st.setIgnored();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusToAct()
{
  Akonadi::MessageStatus st;
  st.setToAct();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusSpam()
{
  Akonadi::MessageStatus st;
  st.setSpam();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusHam()
{
  Akonadi::MessageStatus st;
  st.setHam();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusHasAttachment()
{
  Akonadi::MessageStatus st;
  st.setHasAttachment();
  return st;
}

Akonadi::MessageStatus Akonadi::MessageStatus::statusHasInvitation()
{
  MessageStatus st;
  st.setHasInvitation();
  return st;
}

