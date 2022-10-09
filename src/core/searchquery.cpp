/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchquery.h"
#include "akonadicore_debug.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

using namespace Akonadi;

class Akonadi::SearchTermPrivate : public QSharedData
{
public:
    bool operator==(const SearchTermPrivate &other) const
    {
        return relation == other.relation && isNegated == other.isNegated && terms == other.terms && key == other.key && value == other.value
            && condition == other.condition;
    }

    QString key;
    QVariant value;
    SearchTerm::Condition condition = SearchTerm::CondEqual;
    SearchTerm::Relation relation = SearchTerm::RelAnd;
    QList<SearchTerm> terms;
    bool isNegated = false;
};

class Akonadi::SearchQueryPrivate : public QSharedData
{
public:
    bool operator==(const SearchQueryPrivate &other) const
    {
        return rootTerm == other.rootTerm && limit == other.limit;
    }

    static QVariantMap termToJSON(const SearchTerm &term)
    {
        const QList<SearchTerm> &subTerms = term.subTerms();
        QVariantMap termJSON;
        termJSON.insert(QStringLiteral("negated"), term.isNegated());
        if (subTerms.isEmpty()) {
            if (!term.isNull()) {
                termJSON.insert(QStringLiteral("key"), term.key());
                termJSON.insert(QStringLiteral("value"), term.value());
                termJSON.insert(QStringLiteral("cond"), static_cast<int>(term.condition()));
            }
        } else {
            termJSON.insert(QStringLiteral("rel"), static_cast<int>(term.relation()));
            QVariantList subTermsJSON;
            subTermsJSON.reserve(subTerms.count());
            for (const SearchTerm &term : std::as_const(subTerms)) {
                subTermsJSON.append(termToJSON(term));
            }
            termJSON.insert(QStringLiteral("subTerms"), subTermsJSON);
        }

        return termJSON;
    }

    static SearchTerm JSONToTerm(const QVariantMap &map)
    {
        if (map.isEmpty()) {
            return SearchTerm();
        } else if (map.contains(QStringLiteral("key"))) {
            SearchTerm term(map[QStringLiteral("key")].toString(),
                            map[QStringLiteral("value")],
                            static_cast<SearchTerm::Condition>(map[QStringLiteral("cond")].toInt()));
            term.setIsNegated(map[QStringLiteral("negated")].toBool());
            return term;
        } else if (map.contains(QStringLiteral("rel"))) {
            SearchTerm term(static_cast<SearchTerm::Relation>(map[QStringLiteral("rel")].toInt()));
            term.setIsNegated(map[QStringLiteral("negated")].toBool());
            const QList<QVariant> list = map[QStringLiteral("subTerms")].toList();
            for (const QVariant &var : list) {
                term.addSubTerm(JSONToTerm(var.toMap()));
            }
            return term;
        } else {
            qCWarning(AKONADICORE_LOG) << "Invalid JSON for term: " << map;
            return SearchTerm();
        }
    }

    SearchTerm rootTerm;
    int limit = -1;
};

SearchTerm::SearchTerm(SearchTerm::Relation relation)
    : d(new SearchTermPrivate)
{
    d->relation = relation;
}

SearchTerm::SearchTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition)
    : d(new SearchTermPrivate)
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

SearchTerm::~SearchTerm() = default;

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

QList<SearchTerm> SearchTerm::subTerms() const
{
    return d->terms;
}

SearchTerm::Relation SearchTerm::relation() const
{
    return d->relation;
}

SearchQuery::SearchQuery(SearchTerm::Relation rel)
    : d(new SearchQueryPrivate)
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
    QVariantMap root;
    if (!d->rootTerm.isNull()) {
        root = SearchQueryPrivate::termToJSON(d->rootTerm);
        root.insert(QStringLiteral("limit"), d->limit);
    }

    QJsonObject jo = QJsonObject::fromVariantMap(root);
    QJsonDocument jdoc;
    jdoc.setObject(jo);
    return jdoc.toJson();
}

SearchQuery SearchQuery::fromJSON(const QByteArray &jsonData)
{
    QJsonParseError error;
    const QJsonDocument json = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError || json.isNull()) {
        return SearchQuery();
    }

    SearchQuery query;
    const QJsonObject obj = json.object();
    query.d->rootTerm = SearchQueryPrivate::JSONToTerm(obj.toVariantMap());
    if (obj.contains(QLatin1String("limit"))) {
        query.d->limit = obj.value(QStringLiteral("limit")).toInt();
    }
    return query;
}

static QMap<EmailSearchTerm::EmailSearchField, QString> emailSearchFieldMapping()
{
    static QMap<EmailSearchTerm::EmailSearchField, QString> mapping;
    if (mapping.isEmpty()) {
        mapping.insert(EmailSearchTerm::Body, QStringLiteral("body"));
        mapping.insert(EmailSearchTerm::Headers, QStringLiteral("headers"));
        mapping.insert(EmailSearchTerm::Subject, QStringLiteral("subject"));
        mapping.insert(EmailSearchTerm::Message, QStringLiteral("message"));
        mapping.insert(EmailSearchTerm::HeaderFrom, QStringLiteral("from"));
        mapping.insert(EmailSearchTerm::HeaderTo, QStringLiteral("to"));
        mapping.insert(EmailSearchTerm::HeaderCC, QStringLiteral("cc"));
        mapping.insert(EmailSearchTerm::HeaderBCC, QStringLiteral("bcc"));
        mapping.insert(EmailSearchTerm::HeaderReplyTo, QStringLiteral("replyto"));
        mapping.insert(EmailSearchTerm::HeaderOrganization, QStringLiteral("organization"));
        mapping.insert(EmailSearchTerm::HeaderListId, QStringLiteral("listid"));
        mapping.insert(EmailSearchTerm::HeaderResentFrom, QStringLiteral("resentfrom"));
        mapping.insert(EmailSearchTerm::HeaderXLoop, QStringLiteral("xloop"));
        mapping.insert(EmailSearchTerm::HeaderXMailingList, QStringLiteral("xmailinglist"));
        mapping.insert(EmailSearchTerm::HeaderXSpamFlag, QStringLiteral("xspamflag"));
        mapping.insert(EmailSearchTerm::HeaderDate, QStringLiteral("date"));
        mapping.insert(EmailSearchTerm::HeaderOnlyDate, QStringLiteral("onlydate"));
        mapping.insert(EmailSearchTerm::MessageStatus, QStringLiteral("messagestatus"));
        mapping.insert(EmailSearchTerm::MessageTag, QStringLiteral("messagetag"));
        mapping.insert(EmailSearchTerm::ByteSize, QStringLiteral("size"));
        mapping.insert(EmailSearchTerm::Attachment, QStringLiteral("attachment"));
    }

    return mapping;
}

EmailSearchTerm::EmailSearchTerm(EmailSearchTerm::EmailSearchField field, const QVariant &value, SearchTerm::Condition condition)
    : SearchTerm(toKey(field), value, condition)
{
}

QString EmailSearchTerm::toKey(EmailSearchTerm::EmailSearchField field)
{
    return emailSearchFieldMapping().value(field);
}

EmailSearchTerm::EmailSearchField EmailSearchTerm::fromKey(const QString &key)
{
    return emailSearchFieldMapping().key(key);
}

static QMap<ContactSearchTerm::ContactSearchField, QString> contactSearchFieldMapping()
{
    static QMap<ContactSearchTerm::ContactSearchField, QString> mapping;
    if (mapping.isEmpty()) {
        mapping.insert(ContactSearchTerm::Name, QStringLiteral("name"));
        mapping.insert(ContactSearchTerm::Nickname, QStringLiteral("nickname"));
        mapping.insert(ContactSearchTerm::Email, QStringLiteral("email"));
        mapping.insert(ContactSearchTerm::Uid, QStringLiteral("uid"));
        mapping.insert(ContactSearchTerm::All, QStringLiteral("all"));
    }
    return mapping;
}

ContactSearchTerm::ContactSearchTerm(ContactSearchTerm::ContactSearchField field, const QVariant &value, SearchTerm::Condition condition)
    : SearchTerm(toKey(field), value, condition)
{
}

QString ContactSearchTerm::toKey(ContactSearchTerm::ContactSearchField field)
{
    return contactSearchFieldMapping().value(field);
}

ContactSearchTerm::ContactSearchField ContactSearchTerm::fromKey(const QString &key)
{
    return contactSearchFieldMapping().key(key);
}

QMap<IncidenceSearchTerm::IncidenceSearchField, QString> incidenceSearchFieldMapping()
{
    static QMap<IncidenceSearchTerm::IncidenceSearchField, QString> mapping;
    if (mapping.isEmpty()) {
        mapping.insert(IncidenceSearchTerm::All, QStringLiteral("all"));
        mapping.insert(IncidenceSearchTerm::PartStatus, QStringLiteral("partstatus"));
        mapping.insert(IncidenceSearchTerm::Organizer, QStringLiteral("organizer"));
        mapping.insert(IncidenceSearchTerm::Summary, QStringLiteral("summary"));
        mapping.insert(IncidenceSearchTerm::Location, QStringLiteral("location"));
    }
    return mapping;
}

IncidenceSearchTerm::IncidenceSearchTerm(IncidenceSearchTerm::IncidenceSearchField field, const QVariant &value, SearchTerm::Condition condition)
    : SearchTerm(toKey(field), value, condition)
{
}

QString IncidenceSearchTerm::toKey(IncidenceSearchTerm::IncidenceSearchField field)
{
    return incidenceSearchFieldMapping().value(field);
}

IncidenceSearchTerm::IncidenceSearchField IncidenceSearchTerm::fromKey(const QString &key)
{
    return incidenceSearchFieldMapping().key(key);
}
