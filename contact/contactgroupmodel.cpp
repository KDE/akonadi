/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactgroupmodel_p.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <kabc/addressee.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocalizedstring.h>

using namespace Akonadi;

struct GroupMember
{
  GroupMember()
    : isReference(false), loadingError( false )
  {
  }

  KABC::ContactGroup::ContactReference reference;
  KABC::ContactGroup::Data data;
  KABC::Addressee referencedContact;
  bool isReference;
  bool loadingError;
};

class ContactGroupModel::Private
{
  public:
    Private( ContactGroupModel *parent )
      : mParent( parent )
    {
    }

    void resolveContactReference( const KABC::ContactGroup::ContactReference &reference, int row )
    {
      Item item;
      if ( !reference.gid().isEmpty() ) {
        item.setGid( reference.gid() );
      } else {
        item.setId( reference.uid().toLongLong() );
      }
      ItemFetchJob *job = new ItemFetchJob( item, mParent );
      job->setProperty( "row", row );
      job->fetchScope().fetchFullPayload();

      mParent->connect( job, SIGNAL(result(KJob*)), SLOT(itemFetched(KJob*)) );
    }

    void itemFetched( KJob *job )
    {
      const int row = job->property( "row" ).toInt();

      if ( job->error() ) {
        mMembers[ row ].loadingError = true;
        emit mParent->dataChanged( mParent->index( row, 0, QModelIndex() ), mParent->index( row, 1, QModelIndex() ) );
        return;
      }

      ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

      if ( fetchJob->items().count() != 1 ) {
        mMembers[ row ].loadingError = true;
        emit mParent->dataChanged( mParent->index( row, 0, QModelIndex() ), mParent->index( row, 1, QModelIndex() ) );
        return;
      }

      const Item item = fetchJob->items().first();
      const KABC::Addressee contact = item.payload<KABC::Addressee>();

      GroupMember &member = mMembers[ row ];
      member.referencedContact = contact;
      emit mParent->dataChanged( mParent->index( row, 0, QModelIndex() ), mParent->index( row, 1, QModelIndex() ) );
    }

    void normalizeMemberList()
    {
      // check whether a normalization is needed or not
      bool needsNormalization = false;
      if ( mMembers.isEmpty() ) {
        needsNormalization = true;
      } else {
        for ( int i = 0; i < mMembers.count(); ++i ) {
          const GroupMember &member = mMembers[ i ];
          if ( !member.isReference && !( i == mMembers.count() - 1 ) ) {
            if ( member.data.name().isEmpty() && member.data.email().isEmpty() ) {
              needsNormalization = true;
              break;
            }
          }
        }

        const GroupMember &member = mMembers.last();
        if ( member.isReference || !( member.data.name().isEmpty() && member.data.email().isEmpty() ) ) {
          needsNormalization = true;
        }
      }

      // if not, avoid to update the model and view
      if ( !needsNormalization ) {
        return;
      }

      bool foundEmpty = false;

      // add an empty line at the end
      mParent->beginInsertRows( QModelIndex(), mMembers.count(), mMembers.count() );
      GroupMember member;
      member.isReference = false;
      mMembers.append( member );
      mParent->endInsertRows();

      // remove all empty lines first except the last line
      do {
        foundEmpty = false;
        for ( int i = 0; i < mMembers.count(); ++i ) {
          const GroupMember &member = mMembers[ i ];
          if ( !member.isReference && !( i == mMembers.count() - 1 ) ) {
            if ( member.data.name().isEmpty() && member.data.email().isEmpty() ) {
              mParent->beginRemoveRows( QModelIndex(), i, i );
              mMembers.remove( i );
              mParent->endRemoveRows();
              foundEmpty = true;
              break;
            }
          }
        }
      } while ( foundEmpty );
    }

    ContactGroupModel *mParent;
    QVector<GroupMember> mMembers;
    KABC::ContactGroup mGroup;
    QString mLastErrorMessage;
};

ContactGroupModel::ContactGroupModel( QObject *parent )
  : QAbstractItemModel( parent ), d( new Private( this ) )
{
}

ContactGroupModel::~ContactGroupModel()
{
  delete d;
}

void ContactGroupModel::loadContactGroup( const KABC::ContactGroup &contactGroup )
{
  emit layoutAboutToBeChanged();

  d->mMembers.clear();
  d->mGroup = contactGroup;

  for ( uint i = 0; i < d->mGroup.dataCount(); ++i ) {
    const KABC::ContactGroup::Data data = d->mGroup.data( i );
    GroupMember member;
    member.isReference = false;
    member.data = data;

    d->mMembers.append( member );
  }

  for ( uint i = 0; i < d->mGroup.contactReferenceCount(); ++i ) {
    const KABC::ContactGroup::ContactReference reference = d->mGroup.contactReference( i );
    GroupMember member;
    member.isReference = true;
    member.reference = reference;

    d->mMembers.append( member );

    d->resolveContactReference( reference, d->mMembers.count() - 1 );
  }

  d->normalizeMemberList();

  emit layoutChanged();
}

bool ContactGroupModel::storeContactGroup( KABC::ContactGroup &group ) const
{
  group.removeAllContactReferences();
  group.removeAllContactData();

  for ( int i = 0; i < d->mMembers.count(); ++i ) {
    const GroupMember &member = d->mMembers[ i ];
    if ( member.isReference ) {
      group.append( member.reference );
    } else {
      if ( i != ( d->mMembers.count() - 1 ) ) {
        if ( member.data.email().isEmpty() ) {
          d->mLastErrorMessage =
            i18n( "The member with name <b>%1</b> is missing an email address",
                  member.data.name() );
          return false;
        }
        group.append( member.data );
      }
    }
  }

  return true;
}

QString ContactGroupModel::lastErrorMessage() const
{
  return d->mLastErrorMessage;
}

QModelIndex ContactGroupModel::index( int row, int col, const QModelIndex& ) const
{
  return createIndex( row, col);
}

QModelIndex ContactGroupModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

QVariant ContactGroupModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() ) {
    return QVariant();
  }

  if ( index.row() < 0 || index.row() >= d->mMembers.count() ) {
    return QVariant();
  }

  if ( index.column() < 0 || index.column() > 1 ) {
    return QVariant();
  }

  const GroupMember &member = d->mMembers[ index.row() ];

  if ( role == Qt::DisplayRole ) {
    if ( member.loadingError ) {
      if ( index.column() == 0 ) {
        return i18n( "Contact does not exist any more" );
      } else {
        return QString();
      }
    }

    if ( member.isReference ) {
      if ( index.column() == 0 ) {
        return member.referencedContact.realName();
      } else {
        if ( !member.reference.preferredEmail().isEmpty() ) {
          return member.reference.preferredEmail();
        } else {
          return member.referencedContact.preferredEmail();
        }
      }
    } else {
      if ( index.column() == 0 ) {
        return member.data.name();
      } else {
        return member.data.email();
      }
    }
  }

  if ( role == Qt::DecorationRole ) {
    if ( index.column() == 1 ) {
      return QVariant();
    }

    if ( member.loadingError ) {
      return KIcon( QLatin1String( "emblem-important" ) );
    }

    if ( index.row() == ( d->mMembers.count() - 1 ) ) {
      return KIcon( QLatin1String( "contact-new" ) );
    }

    if ( member.isReference ) {
      return KIcon( QLatin1String( "x-office-contact" ), KIconLoader::global(),
                    QStringList() << QLatin1String( "emblem-symbolic-link" ) );
    } else {
      return KIcon( QLatin1String( "x-office-contact" ) );
    }
  }

  if ( role == Qt::EditRole ) {
    if ( member.isReference ) {
      if ( index.column() == 0 ) {
        return member.referencedContact.realName();
      } else {
        if ( !member.reference.preferredEmail().isEmpty() ) {
          return member.reference.preferredEmail();
        } else {
          return member.referencedContact.preferredEmail();
        }
      }
    } else {
      if ( index.column() == 0 ) {
        return member.data.name();
      } else {
        return member.data.email();
      }
    }
  }

  if ( role == IsReferenceRole ) {
    return member.isReference;
  }

  if ( role == AllEmailsRole ) {
    if ( member.isReference ) {
      return member.referencedContact.emails();
    } else {
      return QStringList();
    }
  }

  return QVariant();
}

bool ContactGroupModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( !index.isValid() ) {
    return false;
  }

  if ( index.row() < 0 || index.row() >= d->mMembers.count() ) {
    return false;
  }

  if ( index.column() < 0 || index.column() > 1 ) {
    return false;
  }

  GroupMember &member = d->mMembers[ index.row() ];

  if ( role == Qt::EditRole ) {
    if ( member.isReference ) {
      if ( index.column() == 0 ) {
        member.reference.setUid( QString::number( value.toLongLong() ) );
        d->resolveContactReference( member.reference, index.row() );
      }
      if ( index.column() == 1 ) {
        const QString email = value.toString();
        if ( email != member.referencedContact.preferredEmail() ) {
          member.reference.setPreferredEmail( email );
        } else {
          member.reference.setPreferredEmail( QString() );
        }
      }
    } else {
      if ( index.column() == 0 ) {
        member.data.setName( value.toString() );
      } else {
        member.data.setEmail( value.toString() );
      }
    }

    d->normalizeMemberList();

    return true;
  }

  if ( role == IsReferenceRole ) {
    if ( ( value.toBool() == true ) && !member.isReference ) {
      member.isReference = true;
    }
    if ( ( value.toBool() == false ) && member.isReference ) {
      member.isReference = false;
      member.data.setName( member.referencedContact.realName() );
      member.data.setEmail( member.referencedContact.preferredEmail() );
    }

    return true;
  }

  return false;
}

QVariant ContactGroupModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section < 0 || section > 1 ) {
    return QVariant();
  }

  if ( orientation != Qt::Horizontal ) {
    return QVariant();
  }

  if ( role != Qt::DisplayRole ) {
    return QVariant();
  }

  if ( section == 0 ) {
    return i18nc( "contact's name", "Name" );
  } else {
    return i18nc( "contact's email address", "EMail" );
  }
}

Qt::ItemFlags ContactGroupModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() || index.row() < 0 || index.row() >= d->mMembers.count() ) {
    return Qt::ItemIsEnabled;
  }

  if ( d->mMembers[ index.row() ].loadingError ) {
    return Qt::ItemFlags( Qt::ItemIsEnabled );
  }

  Qt::ItemFlags parentFlags = QAbstractItemModel::flags( index );
  return ( parentFlags | Qt::ItemIsEnabled | Qt::ItemIsEditable );
}

int ContactGroupModel::columnCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    return 2;
  } else {
    return 0;
  }
}

int ContactGroupModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() ) {
    return d->mMembers.count();
  } else {
    return 0;
  }
}

bool ContactGroupModel::removeRows( int row, int count, const QModelIndex &parent )
{
  if ( parent.isValid() ) {
    return false;
  }

  beginRemoveRows( QModelIndex(), row, row + count - 1 );
  for ( int i = 0; i < count; ++i ) {
    d->mMembers.remove( row );
  }
  endRemoveRows();

  return true;
}

#include "moc_contactgroupmodel_p.cpp"
