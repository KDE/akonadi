/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchquery.h"

#include <QtCore/QVariant>

#include <KDebug>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Akonadi;

class SearchTerm::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , condition(SearchTerm::CondEqual)
        , relation(SearchTerm::RelAnd)
        , isNegated(false)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , key(other.key)
        , value(other.value)
        , condition(other.condition)
        , relation(other.relation)
        , terms(other.terms)
        , isNegated(other.isNegated)
    {
    }

    bool operator==(const Private &other) const
    {
        return relation == other.relation
               && isNegated == other.isNegated
               && terms == other.terms
               && key == other.key
               && value == other.value
               && condition == other.condition;
    }

    QString key;
    QVariant value;
    Condition condition;
    Relation relation;
    QList<SearchTerm> terms;
    bool isNegated;
};

class SearchQuery::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , limit(-1)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
        , rootTerm(other.rootTerm)
        , limit(other.limit)
    {
    }

    bool operator==(const Private &other) const
    {
        return rootTerm == other.rootTerm && limit == other.limit;
    }

    static QVariantMap termToJSON(const SearchTerm &term)
    {
        const QList<SearchTerm> &subTerms = term.subTerms();
        QVariantMap termJSON;
        termJSON.insert(QLatin1String("negated"), term.isNegated());
        if (subTerms.isEmpty()) {
            termJSON.insert(QLatin1String("key"), term.key());
            termJSON.insert(QLatin1String("value"), term.value());
            termJSON.insert(QLatin1String("cond"), static_cast<int>(term.condition()));
        } else {
            termJSON.insert(QLatin1String("rel"), static_cast<int>(term.relation()));
            QVariantList subTermsJSON;
            Q_FOREACH (const SearchTerm &term, subTerms) {
                subTermsJSON.append(termToJSON(term));
            }
            termJSON.insert(QLatin1String("subTerms"), subTermsJSON);
        }

        return termJSON;
    }

    static SearchTerm JSONToTerm(const QVariantMap &json)
    {
        if (json.contains(QLatin1String("key"))) {
            SearchTerm term(json[QLatin1String("key")].toString(),
                            json[QLatin1String("value")],
                            static_cast<SearchTerm::Condition>(json[QLatin1String("cond")].toInt()));
            term.setIsNegated(json[QLatin1String("negated")].toBool());
            return term;
        } else if (json.contains(QLatin1String("rel"))) {
            SearchTerm term(static_cast<SearchTerm::Relation>(json[QLatin1String("rel")].toInt()));
            term.setIsNegated(json[QLatin1String("negated")].toBool());
            const QVariantList subTermsJSON = json[QLatin1String("subTerms")].toList();
            Q_FOREACH (const QVariant &subTermJSON, subTermsJSON) {
                term.addSubTerm(JSONToTerm(subTermJSON.toMap()));
            }
            return term;
        } else {
            kWarning() << "Invalid JSON for term: " << json;
            return SearchTerm();
        }
    }

    SearchTerm rootTerm;
    int limit;
};

SearchTerm::SearchTerm(SearchTerm::Relation relation)
    : d(new Private)
{
    d->relation = relation;
}

SearchTerm::SearchTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition)
    : d(new Private)
{
    d->relation = RelAnd;
    d->key = key;
    d->value = value;
    d->condition = condition;
}

SearchTerm::SearchTerm(const SearchTerm &other)
    : d(other.d)
{
}

SearchTerm::~SearchTerm()
{
}

SearchTerm &SearchTerm::operator=(const SearchTerm &other)
{
    d = other.d;
    return *this;
}

bool SearchTerm::operator==(const SearchTerm &other) const
{
    return *d == *other.d;
}

bool SearchTerm::isNull() const
{
    return d->key.isEmpty() && d->value.isNull() && d->terms.isEmpty();
}

QString SearchTerm::key() const
{
    return d->key;
}

QVariant SearchTerm::value() const
{
    return d->value;
}

SearchTerm::Condition SearchTerm::condition() const
{
    return d->condition;
}

void SearchTerm::setIsNegated(bool negated)
{
    d->isNegated = negated;
}

bool SearchTerm::isNegated() const
{
    return d->isNegated;
}

void SearchTerm::addSubTerm(const SearchTerm &term)
{
    d->terms << term;
}

QList< SearchTerm > SearchTerm::subTerms() const
{
    return d->terms;
}

SearchTerm::Relation SearchTerm::relation() const
{
    return d->relation;
}

SearchQuery::SearchQuery(SearchTerm::Relation rel)
    : d(new Private)
{
    d->rootTerm = SearchTerm(rel);
}

SearchQuery::SearchQuery(const SearchQuery &other)
    : d(other.d)
{
}

SearchQuery::~SearchQuery()
{
}

SearchQuery &SearchQuery::operator=(const SearchQuery &other)
{
    d = other.d;
    return *this;
}

bool SearchQuery::operator==(const SearchQuery &other) const
{
    return *d == *other.d;
}

bool SearchQuery::isNull() const
{
    return d->rootTerm.isNull();
}

SearchTerm SearchQuery::term() const
{
    return d->rootTerm;
}

void SearchQuery::addTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition)
{
    addTerm(SearchTerm(key, value, condition));
}

void SearchQuery::addTerm(const SearchTerm &term)
{
    d->rootTerm.addSubTerm(term);
}

void SearchQuery::setTerm(const SearchTerm &term)
{
    d->rootTerm = term;
}

void SearchQuery::setLimit(int limit)
{
    d->limit = limit;
}

int SearchQuery::limit() const
{
    return d->limit;
}

QByteArray SearchQuery::toJSON() const
{
    QVariantMap root = Private::termToJSON(d->rootTerm);
    root.insert(QLatin1String("limit"), d->limit);

    QJson::Serializer serializer;
    return serializer.serialize(root);
}

SearchQuery SearchQuery::fromJSON(const QByteArray &jsonData)
{
    QJson::Parser parser;
    bool ok = false;
    const QVariant json = parser.parse(jsonData, &ok);
    if (!ok || json.isNull()) {
        return SearchQuery();
    }

    const QVariantMap map = json.toMap();
    SearchQuery query;
    query.d->rootTerm = Private::JSONToTerm(map);
    if (map.contains(QLatin1String("limit"))) {
        query.d->limit = map.value(QLatin1String("limit")).toInt();
    }
    return query;
}

QMap<EmailSearchTerm::EmailSearchField, QString> initializeMapping()
{
    QMap<EmailSearchTerm::EmailSearchField, QString> mapping;
    mapping.insert(EmailSearchTerm::Body, QLatin1String("body"));
    mapping.insert(EmailSearchTerm::Headers, QLatin1String("headers"));
    mapping.insert(EmailSearchTerm::Subject, QLatin1String("subject"));
    mapping.insert(EmailSearchTerm::Message, QLatin1String("message"));
    mapping.insert(EmailSearchTerm::HeaderFrom, QLatin1String("from"));
    mapping.insert(EmailSearchTerm::HeaderTo, QLatin1String("to"));
    mapping.insert(EmailSearchTerm::HeaderCC, QLatin1String("cc"));
    mapping.insert(EmailSearchTerm::HeaderBCC, QLatin1String("bcc"));
    mapping.insert(EmailSearchTerm::HeaderReplyTo, QLatin1String("replyto"));
    mapping.insert(EmailSearchTerm::HeaderOrganization, QLatin1String("organization"));
    mapping.insert(EmailSearchTerm::HeaderListId, QLatin1String("listid"));
    mapping.insert(EmailSearchTerm::HeaderResentFrom, QLatin1String("resentfrom"));
    mapping.insert(EmailSearchTerm::HeaderXLoop, QLatin1String("xloop"));
    mapping.insert(EmailSearchTerm::HeaderXMailingList, QLatin1String("xmailinglist"));
    mapping.insert(EmailSearchTerm::HeaderXSpamFlag, QLatin1String("xspamflag"));
    mapping.insert(EmailSearchTerm::HeaderDate, QLatin1String("date"));
    mapping.insert(EmailSearchTerm::HeaderOnlyDate, QLatin1String("onlydate"));
    mapping.insert(EmailSearchTerm::MessageStatus, QLatin1String("messagestatus"));
    mapping.insert(EmailSearchTerm::MessageTag, QLatin1String("messagetag"));
    mapping.insert(EmailSearchTerm::ByteSize, QLatin1String("size"));
    mapping.insert(EmailSearchTerm::Attachment, QLatin1String("attachment"));
    return mapping;
}

static QMap<EmailSearchTerm::EmailSearchField, QString> emailSearchFieldMapping = initializeMapping();

EmailSearchTerm::EmailSearchTerm(EmailSearchTerm::EmailSearchField field, const QVariant &value, SearchTerm::Condition condition)
    : SearchTerm(toKey(field), value, condition)
{

}

QString EmailSearchTerm::toKey(EmailSearchTerm::EmailSearchField field)
{
    return emailSearchFieldMapping.value(field);
}

EmailSearchTerm::EmailSearchField EmailSearchTerm::fromKey(const QString &key)
{
    return emailSearchFieldMapping.key(key);
}

QMap<ContactSearchTerm::ContactSearchField, QString> initializeContactMapping()
{
    QMap<ContactSearchTerm::ContactSearchField, QString> mapping;
    mapping.insert(ContactSearchTerm::Name, QLatin1String("name"));
    mapping.insert(ContactSearchTerm::Nickname, QLatin1String("nickname"));
    mapping.insert(ContactSearchTerm::Email, QLatin1String("email"));
    mapping.insert(ContactSearchTerm::Uid, QLatin1String("uid"));
    mapping.insert(ContactSearchTerm::All, QLatin1String("all"));
    return mapping;
}

static QMap<ContactSearchTerm::ContactSearchField, QString> contactSearchFieldMapping = initializeContactMapping();

ContactSearchTerm::ContactSearchTerm(ContactSearchTerm::ContactSearchField field, const QVariant &value, SearchTerm::Condition condition)
    : SearchTerm(toKey(field), value, condition)
{

}

QString ContactSearchTerm::toKey(ContactSearchTerm::ContactSearchField field)
{
    return contactSearchFieldMapping.value(field);
}

ContactSearchTerm::ContactSearchField ContactSearchTerm::fromKey(const QString &key)
{
    return contactSearchFieldMapping.key(key);
}

QMap<IncidenceSearchTerm::IncidenceSearchField, QString> initializeIncidenceMapping()
{
  QMap<IncidenceSearchTerm::IncidenceSearchField, QString> mapping;
  mapping.insert(IncidenceSearchTerm::All, QLatin1String("all"));
  mapping.insert(IncidenceSearchTerm::PartStatus, QLatin1String("partstatus"));
  mapping.insert(IncidenceSearchTerm::Organizer, QLatin1String("organizer"));
  mapping.insert(IncidenceSearchTerm::Summary, QLatin1String("summary"));
  mapping.insert(IncidenceSearchTerm::Location, QLatin1String("location"));
  return mapping;
}

static QMap<IncidenceSearchTerm::IncidenceSearchField, QString> incidenceSearchFieldMapping = initializeIncidenceMapping();

IncidenceSearchTerm::IncidenceSearchTerm(IncidenceSearchTerm::IncidenceSearchField field, const QVariant &value, SearchTerm::Condition condition)
: SearchTerm(toKey(field), value, condition)
{

}

QString IncidenceSearchTerm::toKey(IncidenceSearchTerm::IncidenceSearchField field)
{
  return incidenceSearchFieldMapping.value(field);
}

IncidenceSearchTerm::IncidenceSearchField IncidenceSearchTerm::fromKey(const QString &key)
{
  return incidenceSearchFieldMapping.key(key);
}