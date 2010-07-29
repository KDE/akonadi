/*
    This file is part of Akonadi.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

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
    Note, that the KMMsgStatusIgnored implies the
    status to be Read even if the flags are set
    to Unread or New. This is done in isRead()
    and related getters. So we can preserve the state
    when switching a thread to Ignored and back. */
enum MsgStatus {
    KMMsgStatusUnknown =           0x00000000,
    KMMsgStatusUnread =            0x00000002,
    KMMsgStatusRead =              0x00000004,
    KMMsgStatusDeleted =           0x00000010,
    KMMsgStatusReplied =           0x00000020,
    KMMsgStatusForwarded =         0x00000040,
    KMMsgStatusQueued =            0x00000080,
    KMMsgStatusSent =              0x00000100,
    KMMsgStatusFlag =              0x00000200, // flag means important
    KMMsgStatusWatched =           0x00000400,
    KMMsgStatusIgnored =           0x00000800, // forces isRead()
    KMMsgStatusToAct =             0x00001000,
    KMMsgStatusSpam =              0x00002000,
    KMMsgStatusHam =               0x00004000,
    KMMsgStatusHasAttach =         0x00008000,
    KMMsgStatusHasInvitation =     0x00010000
};

Akonadi::MessageStatus::MessageStatus()
{
  mStatus = KMMsgStatusUnknown;
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
  mStatus = KMMsgStatusUnknown;
}

void Akonadi::MessageStatus::set( const Akonadi::MessageStatus &other )
{
  // Those stati are exclusive, but we have to lock at the
  // internal representation because Ignored can manipulate
  // the result of the getter methods.
  if ( other.mStatus & KMMsgStatusUnread ) {
    setUnread();
  }
  if ( other.mStatus & KMMsgStatusRead ) {
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
    setDeleted( !( mStatus & KMMsgStatusDeleted ) );
  }
  if ( other.isReplied() ) {
    setReplied( !( mStatus & KMMsgStatusReplied ) );
  }
  if ( other.isForwarded() ) {
    setForwarded( !( mStatus & KMMsgStatusForwarded ) );
  }
  if ( other.isQueued() ) {
    setQueued( !( mStatus & KMMsgStatusQueued ) );
  }
  if ( other.isSent() ) {
    setSent( !( mStatus & KMMsgStatusSent ) );
  }
  if ( other.isImportant() ) {
    setImportant( !( mStatus & KMMsgStatusFlag ) );
  }

  if ( other.isWatched() ) {
    setWatched( !( mStatus & KMMsgStatusWatched ) );
  }
  if ( other.isIgnored() ) {
    setIgnored( !( mStatus & KMMsgStatusIgnored ) );
  }
  if ( other.isToAct() ) {
    setToAct( !( mStatus & KMMsgStatusToAct ) );
  }
  if ( other.isSpam() ) {
    setSpam( !( mStatus & KMMsgStatusSpam ) );
  }
  if ( other.isHam() ) {
    setHam( !( mStatus & KMMsgStatusHam ) );
  }
  if ( other.hasAttachment() ) {
    setHasAttachment( !( mStatus & KMMsgStatusHasAttach ) );
  }
  if( other.hasInvitation() ) {
    setHasInvitation( !( mStatus & KMMsgStatusHasInvitation ) );
  }
}

bool Akonadi::MessageStatus::isOfUnknownStatus() const
{
  return ( mStatus == KMMsgStatusUnknown );
}

bool Akonadi::MessageStatus::isUnread() const
{
  return ( mStatus & KMMsgStatusUnread && !( mStatus & KMMsgStatusIgnored ) );
}

bool Akonadi::MessageStatus::isRead() const
{
  return ( mStatus & KMMsgStatusRead || mStatus & KMMsgStatusIgnored );
}

bool Akonadi::MessageStatus::isDeleted() const
{
  return ( mStatus & KMMsgStatusDeleted );
}

bool Akonadi::MessageStatus::isReplied() const
{
  return ( mStatus & KMMsgStatusReplied );
}

bool Akonadi::MessageStatus::isForwarded() const
{
  return ( mStatus & KMMsgStatusForwarded );
}

bool Akonadi::MessageStatus::isQueued() const
{
  return ( mStatus & KMMsgStatusQueued );
}

bool Akonadi::MessageStatus::isSent() const
{
   return ( mStatus & KMMsgStatusSent );
}

bool Akonadi::MessageStatus::isImportant() const
{
  return ( mStatus & KMMsgStatusFlag );
}

bool Akonadi::MessageStatus::isWatched() const
{
  return ( mStatus & KMMsgStatusWatched );
}

bool Akonadi::MessageStatus::isIgnored() const
{
  return ( mStatus & KMMsgStatusIgnored );
}

bool Akonadi::MessageStatus::isToAct() const
{
  return ( mStatus & KMMsgStatusToAct );
}

bool Akonadi::MessageStatus::isSpam() const
{
  return ( mStatus & KMMsgStatusSpam );
}

bool Akonadi::MessageStatus::isHam() const
{
  return ( mStatus & KMMsgStatusHam );
}

bool Akonadi::MessageStatus::hasAttachment() const
{
  return ( mStatus & KMMsgStatusHasAttach );
}

bool Akonadi::MessageStatus::hasInvitation() const
{
  return ( mStatus & KMMsgStatusHasInvitation );
}


void Akonadi::MessageStatus::setUnread()
{
  // unread overrides read
  mStatus &= ~KMMsgStatusRead;
  mStatus |= KMMsgStatusUnread;
}

void Akonadi::MessageStatus::setRead()
{
  // Unset unread and set read
  mStatus &= ~KMMsgStatusUnread;
  mStatus |= KMMsgStatusRead;
}

void Akonadi::MessageStatus::setDeleted( bool deleted )
{
  if ( deleted ) {
    mStatus |= KMMsgStatusDeleted;
  } else {
    mStatus &= ~KMMsgStatusDeleted;
  }
}

void Akonadi::MessageStatus::setReplied( bool replied )
{
  if ( replied ) {
    mStatus |= KMMsgStatusReplied;
  } else {
    mStatus &= ~KMMsgStatusReplied;
  }
}

void Akonadi::MessageStatus::setForwarded( bool forwarded )
{
  if ( forwarded ) {
    mStatus |= KMMsgStatusForwarded;
  } else {
    mStatus &= ~KMMsgStatusForwarded;
  }
}

void Akonadi::MessageStatus::setQueued( bool queued )
{
  if ( queued ) {
    mStatus |= KMMsgStatusQueued;
  } else {
    mStatus &= ~KMMsgStatusQueued;
  }
}

void Akonadi::MessageStatus::setSent( bool sent )
{
  if ( sent ) {
    mStatus &= ~KMMsgStatusQueued;
    // FIXME to be discussed if sent messages are Read
    mStatus &= ~KMMsgStatusUnread;
    mStatus |= KMMsgStatusSent;
  } else {
    mStatus &= ~KMMsgStatusSent;
  }
}

void Akonadi::MessageStatus::setImportant( bool important )
{
  if ( important ) {
    mStatus |= KMMsgStatusFlag;
  } else {
    mStatus &= ~KMMsgStatusFlag;
  }
}

// Watched and ignored are mutually exclusive
void Akonadi::MessageStatus::setWatched( bool watched )
{
  if ( watched ) {
    mStatus &= ~KMMsgStatusIgnored;
    mStatus |= KMMsgStatusWatched;
  } else {
    mStatus &= ~KMMsgStatusWatched;
  }
}

void Akonadi::MessageStatus::setIgnored( bool ignored )
{
  if ( ignored ) {
    mStatus &= ~KMMsgStatusWatched;
    mStatus |= KMMsgStatusIgnored;
  } else {
    mStatus &= ~KMMsgStatusIgnored;
  }
}

void Akonadi::MessageStatus::setToAct( bool toAct )
{
  if ( toAct ) {
    mStatus |= KMMsgStatusToAct;
  } else {
    mStatus &= ~KMMsgStatusToAct;
  }
}

// Ham and Spam are mutually exclusive
void Akonadi::MessageStatus::setSpam( bool spam )
{
  if ( spam ) {
    mStatus &= ~KMMsgStatusHam;
    mStatus |= KMMsgStatusSpam;
  } else {
    mStatus &= ~KMMsgStatusSpam;
  }
}

void Akonadi::MessageStatus::setHam( bool ham )
{
  if ( ham ) {
    mStatus &= ~KMMsgStatusSpam;
    mStatus |= KMMsgStatusHam;
  } else {
    mStatus &= ~KMMsgStatusHam;
  }
}

void Akonadi::MessageStatus::setHasAttachment( bool withAttachment )
{
  if ( withAttachment ) {
    mStatus |= KMMsgStatusHasAttach;
  } else {
    mStatus &= ~KMMsgStatusHasAttach;
  }
}

void Akonadi::MessageStatus::setHasInvitation( bool withInvitation )
{
  if ( withInvitation ) {
    mStatus |= KMMsgStatusHasInvitation;
  } else {
    mStatus &= ~KMMsgStatusHasInvitation;
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
  if ( mStatus & KMMsgStatusUnread ) {
    sstr += 'U';
  }
  if ( mStatus & KMMsgStatusRead ) {
    sstr += 'R';
  }
  if ( mStatus & KMMsgStatusDeleted ) {
    sstr += 'D';
  }
  if ( mStatus & KMMsgStatusReplied ) {
    sstr += 'A';
  }
  if ( mStatus & KMMsgStatusForwarded ) {
    sstr += 'F';
  }
  if ( mStatus & KMMsgStatusQueued ) {
    sstr += 'Q';
  }
  if ( mStatus & KMMsgStatusToAct ) {
    sstr += 'K';
  }
  if ( mStatus & KMMsgStatusSent ) {
    sstr += 'S';
  }
  if ( mStatus & KMMsgStatusFlag ) {
    sstr += 'G';
  }
  if ( mStatus & KMMsgStatusWatched ) {
    sstr += 'W';
  }
  if ( mStatus & KMMsgStatusIgnored ) {
    sstr += 'I';
  }
  if ( mStatus & KMMsgStatusSpam ) {
    sstr += 'P';
  }
  if ( mStatus & KMMsgStatusHam ) {
    sstr += 'H';
  }
  if ( mStatus & KMMsgStatusHasAttach ) {
    sstr += 'T';
  }

  return QLatin1String( sstr );
}

void Akonadi::MessageStatus::setStatusFromStr( const QString& aStr )
{
  mStatus = KMMsgStatusUnknown;

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
  // * KMMsgStatusQueued
  // * KMMsgStatusSent
  // * KMMsgStatusSpam
  // * KMMsgStatusHam
  // * KMMsgStatusHasAttach

  if ( mStatus & KMMsgStatusDeleted ) {
    flags+= Akonadi::MessageFlags::Deleted;
  } else {
    if ( mStatus &  KMMsgStatusRead )
      flags+= Akonadi::MessageFlags::Seen;
    if ( mStatus & KMMsgStatusReplied )
      flags+= Akonadi::MessageFlags::Answered;
    if ( mStatus & KMMsgStatusFlag )
      flags+= Akonadi::MessageFlags::Flagged;
    // non standard flags
    if ( mStatus & KMMsgStatusForwarded )
      flags+= "$FORWARDED";
    if ( mStatus & KMMsgStatusToAct )
      flags+= "$TODO";
    if ( mStatus & KMMsgStatusWatched )
      flags+= "$WATCHED";
    if ( mStatus & KMMsgStatusIgnored )
      flags+= "$IGNORED";
    if ( mStatus & KMMsgStatusHasAttach )
      flags+= "$ATTACHMENT";
  }

  return flags;
}

void Akonadi::MessageStatus::setStatusFromFlags( const QSet<QByteArray> &flags )
{
  mStatus = KMMsgStatusUnknown;
  setUnread();
  // Non handled status:
  // * KMMsgStatusQueued
  // * KMMsgStatusSent
  // * KMMsgStatusSpam
  // * KMMsgStatusHam
  // * KMMsgStatusHasAttach

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
    } else if ( upperedFlag ==  "$ATTACHMENT" ) {
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

