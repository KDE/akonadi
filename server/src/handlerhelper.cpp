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

#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionstatistics.h"
#include "storage/queryhelper.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "commandcontext.h"
#include "handler.h"

#include <QtSql/QSqlError>

using namespace Akonadi;
using namespace Akonadi::Server;

Collection HandlerHelper::collectionFromIdOrName( const QByteArray &id )
{
  // id is a number
  bool ok = false;
  qint64 collectionId = id.toLongLong( &ok );
  if ( ok ) {
    return Collection::retrieveById( collectionId );
  }

  // id is a path
  QString path = QString::fromUtf8( id ); // ### should be UTF-7 for real IMAP compatibility

  const QStringList pathParts = path.split( QLatin1Char( '/' ), QString::SkipEmptyParts );
  Collection col;
  Q_FOREACH ( const QString &part, pathParts ) {
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition( Collection::nameColumn(), Query::Equals, part );
    if ( col.isValid() ) {
      qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, col.id() );
    } else {
      qb.addValueCondition( Collection::parentIdColumn(), Query::Is, QVariant() );
    }
    if ( !qb.exec() ) {
      return Collection();
    }
    Collection::List list = qb.result();
    if ( list.count() != 1 ) {
      return Collection();
    }
    col = list.first();
  }
  return col;
}

QString HandlerHelper::pathForCollection( const Collection &col )
{
  QStringList parts;
  Collection current = col;
  while ( current.isValid() ) {
    parts.prepend( current.name() );
    current = current.parent();
  }
  return parts.join( QLatin1String( "/" ) );
}

int HandlerHelper::parseCachePolicy( const QByteArray &data, Collection &col, int start, bool *changed )
{
  bool inheritChanged = false;
  bool somethingElseChanged = false;

  QList<QByteArray> params;
  int end = ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == AKONADI_PARAM_INHERIT ) {
      const bool inherit = value == "true";
      inheritChanged = col.cachePolicyInherit() != inherit;
      col.setCachePolicyInherit( inherit );
    } else if ( key == AKONADI_PARAM_INTERVAL ) {
      const int interval = value.toInt();
      somethingElseChanged = somethingElseChanged || interval != col.cachePolicyCheckInterval();
      col.setCachePolicyCheckInterval( interval );
    } else if ( key == AKONADI_PARAM_CACHETIMEOUT ) {
      const int timeout = value.toInt();
      somethingElseChanged = somethingElseChanged || timeout != col.cachePolicyCacheTimeout();
      col.setCachePolicyCacheTimeout( timeout );
    } else if ( key == AKONADI_PARAM_SYNCONDEMAND ) {
      const bool syncOnDemand = value == "true";
      somethingElseChanged = somethingElseChanged || syncOnDemand != col.cachePolicySyncOnDemand();
      col.setCachePolicySyncOnDemand( syncOnDemand );
    } else if ( key == AKONADI_PARAM_LOCALPARTS ) {
      QList<QByteArray> tmp;
      QStringList partsList;
      ImapParser::parseParenthesizedList( value, tmp );
      Q_FOREACH ( const QByteArray &ba, tmp ) {
        partsList << QString::fromLatin1( ba );
      }
      const QString parts = partsList.join( QLatin1String( " " ) );
      somethingElseChanged = somethingElseChanged || col.cachePolicyLocalParts() != parts;
      col.setCachePolicyLocalParts( parts );
    }
  }

  if ( changed && ( inheritChanged || ( !col.cachePolicyInherit() && somethingElseChanged ) ) ) {
    *changed = true;
  }

  return end;
}

QByteArray HandlerHelper::cachePolicyToByteArray( const Collection &col )
{
  QByteArray rv = AKONADI_PARAM_CACHEPOLICY " (";
  rv += AKONADI_PARAM_INHERIT " " + ( col.cachePolicyInherit() ? QByteArray( "true" ) : QByteArray( "false" ) );
  rv += " " AKONADI_PARAM_INTERVAL " " + QByteArray::number( col.cachePolicyCheckInterval() );
  rv += " " AKONADI_PARAM_CACHETIMEOUT " " + QByteArray::number( col.cachePolicyCacheTimeout() );
  rv += " " AKONADI_PARAM_SYNCONDEMAND " " + ( col.cachePolicySyncOnDemand() ? QByteArray( "true" ) : QByteArray( "false" ) );
  rv += " " AKONADI_PARAM_LOCALPARTS " (" + col.cachePolicyLocalParts().toLatin1() + ')';
  rv += ')';
  return rv;
}

QByteArray HandlerHelper::tristateToByteArray( const Tristate &tristate )
{
  if ( tristate == Tristate::True ) {
    return "TRUE";
  } else if ( tristate == Tristate::False ) {
    return "FALSE";
  }
  return "DEFAULT";
}

QByteArray HandlerHelper::collectionToByteArray( const Collection &col, bool hidden, bool includeStatistics,
                                                 int ancestorDepth, const QStack<Collection> &ancestors, bool isReferenced )
{
  QByteArray b = QByteArray::number( col.id() ) + ' '
               + QByteArray::number( col.parentId() ) + " (";

  b += AKONADI_PARAM_NAME " " + ImapParser::quote( col.name().toUtf8() ) + ' ';
  if ( hidden ) {
    b += AKONADI_PARAM_MIMETYPE " () ";
  } else {
    b += AKONADI_PARAM_MIMETYPE " (" + MimeType::joinByName( col.mimeTypes(), QLatin1String( " " ) ).toLatin1() + ") ";
  }
  b += AKONADI_PARAM_REMOTEID " " + ImapParser::quote( col.remoteId().toUtf8() );
  b += " " AKONADI_PARAM_REMOTEREVISION " " + ImapParser::quote( col.remoteRevision().toUtf8() );
  b += " " AKONADI_PARAM_RESOURCE " " + ImapParser::quote( col.resource().name().toUtf8() );
  b += " " AKONADI_PARAM_VIRTUAL " " + QByteArray::number( col.isVirtual() ) + ' ';

  if ( includeStatistics ) {
    const CollectionStatistics::Statistics &stats = CollectionStatistics::self()->statistics(col);
    if (stats.count > -1) {
      b += AKONADI_ATTRIBUTE_MESSAGES " " + QByteArray::number( stats.count ) + ' ';
      b += AKONADI_ATTRIBUTE_UNSEEN " ";
      b += QByteArray::number( stats.count - stats.read) ;
      b += " " AKONADI_PARAM_SIZE " " + QByteArray::number( stats.size ) + ' ';
    }
  }

  if ( !col.queryString().isEmpty() ) {
    b += AKONADI_PARAM_PERSISTENTSEARCH " ";
    QList<QByteArray> args;
    args.append( col.queryAttributes().toLatin1() );
    args.append( AKONADI_PARAM_PERSISTENTSEARCH_QUERYSTRING );
    args.append( ImapParser::quote( col.queryString().toUtf8() ) );
    args.append( AKONADI_PARAM_PERSISTENTSEARCH_QUERYCOLLECTIONS );
    args.append( "(" + col.queryCollections().toLatin1() + ")" );
    b += ImapParser::quote( "(" + ImapParser::join( args, " " ) + ")" );
    b += ' ';
  }

  b += HandlerHelper::cachePolicyToByteArray( col ) + ' ';
  if ( ancestorDepth > 0 ) {
    b += HandlerHelper::ancestorsToByteArray( ancestorDepth, ancestors ) + ' ';
  }

  if ( isReferenced ) {
    b += AKONADI_PARAM_REFERENCED " TRUE ";
  }
  b += AKONADI_PARAM_ENABLED " ";
  if ( col.enabled() ) {
    b += "TRUE ";
  } else {
    b += "FALSE ";
  }
  b += AKONADI_PARAM_DISPLAY " " + tristateToByteArray( col.displayPref() ) + ' ';
  b += AKONADI_PARAM_SYNC " " + tristateToByteArray( col.syncPref() ) + ' ';
  b += AKONADI_PARAM_INDEX " " + tristateToByteArray( col.indexPref() ) + ' ';

  const CollectionAttribute::List attrs = col.attributes();
  for ( int i = 0; i < attrs.size(); ++i ) {
    const CollectionAttribute &attr = attrs[i];
    //Workaround to skip invalid "PARENT " attributes that were accidentaly created after 6e5bbf6
    if ( attr.type() == "PARENT" ) {
      continue;
    }
    b += attr.type() + ' ' + ImapParser::quote( attr.value() );
    if ( i != attrs.size() - 1 ) {
      b += ' ';
    }
  }
  b+= ')';

  return b;
}

QByteArray HandlerHelper::ancestorsToByteArray( int ancestorDepth, const QStack<Collection> &_ancestors )
{
  QByteArray b;
  if ( ancestorDepth > 0 ) {
    b += AKONADI_PARAM_ANCESTORS " (";
    QStack<Collection> ancestors( _ancestors );
    for ( int i = 0; i < ancestorDepth; ++i ) {
      if ( ancestors.isEmpty() ) {
        b += "(0 \"\")";
        break;
      }
      b += '(';
      const Collection c = ancestors.pop();
      b += QByteArray::number( c.id() ) + " ";
      b += ImapParser::quote( c.remoteId().toUtf8() );
      b += ")";
      if ( i != ancestorDepth - 1 ) {
        b += ' ';
      }
    }
    b += ')';
  }
  return b;
}

int HandlerHelper::parseDepth( const QByteArray &depth )
{
  if ( depth.isEmpty() ) {
    throw ImapParserException( "No depth specified" );
  }
  if ( depth == "INF" ) {
    return INT_MAX;
  }
  bool ok = false;
  int result = depth.toInt( &ok );
  if ( !ok ) {
    throw ImapParserException( "Invalid depth argument" );
  }
  return result;
}

Flag::List HandlerHelper::resolveFlags( const QVector<QByteArray> &flagNames )
{
  Flag::List flagList;
  Q_FOREACH ( const QByteArray &flagName, flagNames ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( flagName ) );
    if ( !flag.isValid() ) {
      flag = Flag( QString::fromUtf8( flagName ) );
      if ( !flag.insert() ) {
        throw HandlerException( "Unable to create flag" );
      }
    }
    flagList.append( flag );
  }
  return flagList;
}

Tag::List HandlerHelper::resolveTags( const ImapSet &tags )
{
  SelectQueryBuilder<Tag> qb;
  QueryHelper::setToQuery( tags, Tag::idFullColumnName(), qb );
  if ( !qb.exec() ) {
    throw HandlerException( "Unable to resolve tags" );
  }
  const Tag::List result = qb.result();
  if ( result.isEmpty() ) {
    throw HandlerException( "No tags found" );
  }
  return result;
}

Tag::List HandlerHelper::resolveTagsByGID(const QVector<QByteArray> &tagsGIDs)
{
    Tag::List tagList;
    if (tagsGIDs.isEmpty()) {
        return tagList;
    }

    Q_FOREACH (const QByteArray &tagGID, tagsGIDs) {
        Tag::List tags = Tag::retrieveFiltered(Tag::gidColumn(), QString::fromLatin1(tagGID));
        Tag tag;
        if (tags.isEmpty()) {
            tag.setGid(QString::fromUtf8(tagGID));
            tag.setParentId(0);

            TagType type = TagType::retrieveByName(QLatin1String("PLAIN"));
            if (!type.isValid()) {
                type.setName(QLatin1String("PLAIN"));
                if (!type.insert()) {
                    throw HandlerException("Unable to create tag type");
                }
            }
            tag.setTagType(type);
            if (!tag.insert()) {
                throw HandlerException("Unable to create tag");
            }
        } else if (tags.count() == 1) {
            tag = tags[0];
        } else {
            // Should not happen
            throw HandlerException("Tag GID is not unique");
        }

        tagList.append(tag);
    }

    return tagList;
}

Tag::List HandlerHelper::resolveTagsByRID(const QVector< QByteArray >& tagsRIDs, CommandContext* context)
{
    Tag::List tags;
    if (tagsRIDs.isEmpty()) {
        return tags;
    }

    if (!context->resource().isValid()) {
        throw HandlerException("Tags can be resolved by their RID only in resource context");
    }

    Q_FOREACH (const QByteArray &tagRID, tagsRIDs) {
        SelectQueryBuilder<Tag> qb;
        Query::Condition cond;
        cond.addColumnCondition(Tag::idFullColumnName(), Query::Equals, TagRemoteIdResourceRelation::tagIdFullColumnName());
        cond.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context->resource().id());
        qb.addJoin(QueryBuilder::LeftJoin, TagRemoteIdResourceRelation::tableName(), cond);
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::Equals, QString::fromLatin1(tagRID));
        if (!qb.exec()) {
            throw HandlerException("Unable to resolve tags");
        }

        Tag tag;
        Tag::List results = qb.result();
        if (results.isEmpty()) {
            // If the tag does not exist, we create a new one with GID matching RID
            Tag::List tags = resolveTagsByGID(QVector<QByteArray>() << tagRID);
            if (tags.count() != 1) {
                throw HandlerException("Unable to resolve tag");
            }
            tag = tags[0];
            TagRemoteIdResourceRelation rel;
            rel.setRemoteId(QString::fromUtf8(tagRID));
            rel.setTagId(tag.id());
            rel.setResourceId(context->resource().id());
            if (!rel.insert()) {
                throw HandlerException("Unable to create tag");
            }
        } else if (results.count() == 1) {
            tag = results[0];
        } else {
            throw HandlerException("Tag RID is not unique within this resource context");
        }

        tags.append(tag);
    }

    return tags;
}
