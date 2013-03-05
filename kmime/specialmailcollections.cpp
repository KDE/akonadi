/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialmailcollections.h"
#include "akonadi/specialcollectionattribute_p.h"
#include "akonadi/entitydisplayattribute.h"
#include "akonadi/collectionmodifyjob.h"
#include "specialmailcollectionssettings.h"

#include <KGlobal>
#include <KLocalizedString>
#include "akonadi/agentinstance.h"
#include "akonadi/servermanager.h"

using namespace Akonadi;

class Akonadi::SpecialMailCollectionsPrivate
{
  public:
    SpecialMailCollectionsPrivate();
    ~SpecialMailCollectionsPrivate();

    SpecialMailCollections *mInstance;
};

typedef SpecialMailCollectionsSettings Settings;

K_GLOBAL_STATIC( SpecialMailCollectionsPrivate, sInstance )

static const char s_specialCollectionTypes[SpecialMailCollections::LastType][11] = {
    "local-mail",
    "inbox",
    "outbox",
    "sent-mail",
    "trash",
    "drafts",
    "templates"
};

static const int s_numTypes = sizeof s_specialCollectionTypes / sizeof *s_specialCollectionTypes;

BOOST_STATIC_ASSERT(s_numTypes == SpecialMailCollections::LastType);

static inline QByteArray enumToType( SpecialMailCollections::Type value )
{
    return s_specialCollectionTypes[value];
}

static inline SpecialMailCollections::Type typeToEnum(const QByteArray &type)
{
    for (int i = 0; i < s_numTypes; ++i) {
        if (type == s_specialCollectionTypes[i]) {
            return static_cast<SpecialMailCollections::Type>(i);
        }
    }
    return SpecialMailCollections::Invalid;
}

SpecialMailCollectionsPrivate::SpecialMailCollectionsPrivate()
  : mInstance( new SpecialMailCollections( this ) )
{
}

SpecialMailCollectionsPrivate::~SpecialMailCollectionsPrivate()
{
  delete mInstance;
}

static KCoreConfigSkeleton *getConfig( const QString &filename)
{
  Settings::instance( ServerManager::addNamespace(filename) );
  return Settings::self();
}

SpecialMailCollections::SpecialMailCollections( SpecialMailCollectionsPrivate *dd )
  : SpecialCollections( getConfig(QLatin1String("specialmailcollectionsrc")) ),
    d( dd )
{
}

SpecialMailCollections *SpecialMailCollections::self()
{
  return sInstance->mInstance;
}

bool SpecialMailCollections::hasCollection( Type type, const AgentInstance &instance ) const
{
  return SpecialCollections::hasCollection( enumToType( type ), instance );
}

Collection SpecialMailCollections::collection( Type type, const AgentInstance &instance ) const
{
  return SpecialCollections::collection( enumToType( type ), instance );
}

bool SpecialMailCollections::registerCollection( Type type, const Collection &collection )
{
  return SpecialCollections::registerCollection( enumToType( type ), collection );
}

bool SpecialMailCollections::hasDefaultCollection( Type type ) const
{
  return SpecialCollections::hasDefaultCollection( enumToType( type ) );
}

Collection SpecialMailCollections::defaultCollection( Type type ) const
{
  return SpecialCollections::defaultCollection( enumToType( type ) );
}

void SpecialMailCollections::verifyI18nDefaultCollection( Type type )
{
  Collection collection = defaultCollection( type );
  QString defaultI18n;

  switch ( type ) {
  case SpecialMailCollections::Inbox:
    defaultI18n = i18nc( "local mail folder", "inbox" );
    break;
  case SpecialMailCollections::Outbox:
    defaultI18n = i18nc( "local mail folder", "outbox" );
    break;
  case SpecialMailCollections::SentMail:
    defaultI18n = i18nc( "local mail folder", "sent-mail" );
    break;
  case SpecialMailCollections::Trash:
     defaultI18n = i18nc( "local mail folder", "trash" );
     break;
  case SpecialMailCollections::Drafts:
     defaultI18n = i18nc( "local mail folder", "drafts" );
     break;
  case SpecialMailCollections::Templates:
     defaultI18n = i18nc( "local mail folder", "templates" );
     break;
  default:
     break;
  }
  if(!defaultI18n.isEmpty()) {
    if(collection.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
      if( collection.attribute<Akonadi::EntityDisplayAttribute>()->displayName() != defaultI18n) {
          collection.attribute<Akonadi::EntityDisplayAttribute>()->setDisplayName( defaultI18n );
          Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( collection, this );
          connect( job, SIGNAL(result(KJob*)), this, SLOT(slotCollectionModified(KJob*)) );
      }
    }
  }
}

void SpecialMailCollections::slotCollectionModified(KJob*job)
{
  if ( job->error() ) {
    kDebug()<<" Error when we modified collection";
    return;
  }
}

SpecialMailCollections::Type SpecialMailCollections::specialCollectionType(const Akonadi::Collection &collection)
{
    if (!collection.hasAttribute<SpecialCollectionAttribute>()) {
        return Invalid;
    } else {
        return typeToEnum(collection.attribute<SpecialCollectionAttribute>()->collectionType());
    }
}

