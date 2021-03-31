/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSharedPointer>

#include "akonadicore_export.h"

namespace Akonadi
{
/**
 * Search term represents the actual condition within query.
 *
 * SearchTerm can either have multiple subterms, or can be so-called endterm, when
 * there are no more subterms, but instead the actual condition is specified, that
 * is have key, value and relation between them.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT SearchTerm
{
public:
    enum Relation {
        RelAnd,
        RelOr,
    };

    enum Condition {
        CondEqual,
        CondGreaterThan,
        CondGreaterOrEqual,
        CondLessThan,
        CondLessOrEqual,
        CondContains,
    };

    /**
     * Constructs a term where all subterms will be in given relation
     */
    explicit SearchTerm(SearchTerm::Relation relation = SearchTerm::RelAnd);

    /**
     * Constructs an end term
     */
    SearchTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    SearchTerm(const SearchTerm &other);
    ~SearchTerm();

    SearchTerm &operator=(const SearchTerm &other);
    Q_REQUIRED_RESULT bool operator==(const SearchTerm &other) const;

    Q_REQUIRED_RESULT bool isNull() const;

    /**
     * Returns key of this end term.
     */
    Q_REQUIRED_RESULT QString key() const;

    /**
     * Returns value of this end term.
     */
    Q_REQUIRED_RESULT QVariant value() const;

    /**
     * Returns relation between key and value.
     */
    Q_REQUIRED_RESULT SearchTerm::Condition condition() const;

    /**
     * Adds a new subterm to this term.
     *
     * Subterms will be in relation as specified in SearchTerm constructor.
     *
     * If there are subterms in a term, key, value and condition are ignored.
     */
    void addSubTerm(const SearchTerm &term);

    /**
     * Returns all subterms, or an empty list if this is an end term.
     */
    Q_REQUIRED_RESULT QList<SearchTerm> subTerms() const;

    /**
     * Returns relation in which all subterms are.
     */
    Q_REQUIRED_RESULT SearchTerm::Relation relation() const;

    /**
     * Sets whether the entire term is negated.
     */
    void setIsNegated(bool negated);

    /**
     * Returns whether the entire term is negated.
     */
    Q_REQUIRED_RESULT bool isNegated() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

/**
 * @brief A query that can be passed to ItemSearchJob or others.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT SearchQuery
{
public:
    /**
     * Constructs query where all added terms will be in given relation
     */
    explicit SearchQuery(SearchTerm::Relation rel = SearchTerm::RelAnd);

    ~SearchQuery();
    SearchQuery(const SearchQuery &other);
    SearchQuery &operator=(const SearchQuery &other);
    bool operator==(const SearchQuery &other) const;

    bool isNull() const;

    /**
     * Adds a new term.
     */
    void addTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /**
     * Adds a new term with subterms
     */
    void addTerm(const SearchTerm &term);

    /**
     * Sets the root term
     */
    void setTerm(const SearchTerm &term);

    /**
     * Returns the root term.
     */
    SearchTerm term() const;

    /**
     * Sets the maximum number of results.
     *
     * Note that this limit is only evaluated per search backend,
     * so the total number of results retrieved may be larger.
     */
    void setLimit(int limit);

    /**
     * Returns the maximum number of results.
     *
     * The default value is -1, indicating no limit.
     */
    int limit() const;

    QByteArray toJSON() const;
    static SearchQuery fromJSON(const QByteArray &json);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

/**
 * A search term for an email field.
 *
 * This class can be used to create queries that akonadi email search backends understand.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT EmailSearchTerm : public SearchTerm
{
public:
    /**
     * All fields expect a search string unless noted otherwise.
     */
    enum EmailSearchField {
        Unknown,
        Subject,
        Body,
        Message, // Complete message including headers, body and attachment
        Headers, // All headers
        HeaderFrom,
        HeaderTo,
        HeaderCC,
        HeaderBCC,
        HeaderReplyTo,
        HeaderOrganization,
        HeaderListId,
        HeaderResentFrom,
        HeaderXLoop,
        HeaderXMailingList,
        HeaderXSpamFlag,
        HeaderDate, // Expects QDateTime
        HeaderOnlyDate, // Expectes QDate
        MessageStatus, // Expects message flag from Akonadi::MessageFlags. Boolean filter.
        ByteSize, // Expects int
        Attachment, // Textsearch on attachment
        MessageTag
    };

    /**
     * Constructs an email end term
     */
    EmailSearchTerm(EmailSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /**
     * Translates field to key
     */
    static QString toKey(EmailSearchField);

    /**
     * Translates key to field
     */
    static EmailSearchField fromKey(const QString &key);
};

/**
 * A search term for a contact field.
 *
 * This class can be used to create queries that akonadi contact search backends understand.
 *
 * @since 4.13
 */
class AKONADICORE_EXPORT ContactSearchTerm : public SearchTerm
{
public:
    enum ContactSearchField {
        Unknown,
        Name,
        Email,
        Nickname,
        Uid,
        All // Special field: matches all contacts.
    };

    ContactSearchTerm(ContactSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /**
     * Translates field to key
     */
    static QString toKey(ContactSearchField);

    /**
     * Translates key to field
     */
    static ContactSearchField fromKey(const QString &key);
};

/**
 * A search term for a incidence field.
 *
 * This class can be used to create queries that akonadi incidence search backends understand.
 *
 * @since 5.0
 */
class AKONADICORE_EXPORT IncidenceSearchTerm : public SearchTerm
{
public:
    enum IncidenceSearchField {
        Unknown,
        All,
        PartStatus, // Own PartStatus
        Organizer,
        Summary,
        Location
    };

    IncidenceSearchTerm(IncidenceSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /**
     * Translates field to key
     */
    static QString toKey(IncidenceSearchField);

    /**
     * Translates key to field
     */
    static IncidenceSearchField fromKey(const QString &key);
};

}

