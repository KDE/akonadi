/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSharedPointer>

#include "akonadicore_export.h"

namespace Akonadi
{
class SearchTermPrivate;

/*!
 * \class Akonadi::SearchTerm
 * \inheaderfile Akonadi/SearchQuery
 * \inmodule AkonadiCore
 *
 * Search term represents the actual condition within query.
 *
 * SearchTerm can either have multiple subterms, or can be so-called endterm, when
 * there are no more subterms, but instead the actual condition is specified, that
 * is have key, value and relation between them.
 *
 * \since 4.13
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

    enum SearchField {
        Unknown = 0, ///< Invalid search field
        Collection = 1 ///< Match entities from given Collection (expects Collection ID (qint64))
    };

    /*!
     * Constructs a term where all subterms will be in given relation
     */
    explicit SearchTerm(SearchTerm::Relation relation = SearchTerm::RelAnd);

    /*!
     * Constructs an end term
     */
    SearchTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);
    SearchTerm(SearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);
    SearchTerm(const SearchTerm &other);
    ~SearchTerm();

    SearchTerm &operator=(const SearchTerm &other);
    [[nodiscard]] bool operator==(const SearchTerm &other) const;

    [[nodiscard]] bool isNull() const;

    /*!
     * Returns key of this end term.
     */
    [[nodiscard]] QString key() const;

    /*!
     * Returns value of this end term.
     */
    [[nodiscard]] QVariant value() const;

    /*!
     * Returns relation between key and value.
     */
    [[nodiscard]] SearchTerm::Condition condition() const;

    /*!
     * Adds a new subterm to this term.
     *
     * Subterms will be in relation as specified in SearchTerm constructor.
     *
     * If there are subterms in a term, key, value and condition are ignored.
     */
    void addSubTerm(const SearchTerm &term);

    /*!
     * Returns all subterms, or an empty list if this is an end term.
     */
    [[nodiscard]] QList<SearchTerm> subTerms() const;

    /*!
     * Returns relation in which all subterms are.
     */
    [[nodiscard]] SearchTerm::Relation relation() const;

    /*!
     * Sets whether the entire term is negated.
     */
    void setIsNegated(bool negated);

    /*!
     * Returns whether the entire term is negated.
     */
    [[nodiscard]] bool isNegated() const;

    static QString toKey(SearchField field);
    static SearchField fromKey(const QString &key);

private:
    QSharedDataPointer<SearchTermPrivate> d;
};

class SearchQueryPrivate;

/*!
 * \class Akonadi::SearchQuery
 * \inheaderfile Akonadi/SearchQuery
 * \inmodule AkonadiCore
 *
 * \brief A query that can be passed to ItemSearchJob or others.
 *
 * \since 4.13
 */
class AKONADICORE_EXPORT SearchQuery
{
public:
    /*!
     * Constructs query where all added terms will be in given relation
     */
    explicit SearchQuery(SearchTerm::Relation rel = SearchTerm::RelAnd);

    ~SearchQuery();
    SearchQuery(const SearchQuery &other);
    SearchQuery &operator=(const SearchQuery &other);
    bool operator==(const SearchQuery &other) const;

    bool isNull() const;

    /*!
     * Adds a new term.
     */
    void addTerm(const QString &key, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /*!
     * Adds a new term with subterms
     */
    void addTerm(const SearchTerm &term);

    /*!
     * Sets the root term
     */
    void setTerm(const SearchTerm &term);

    /*!
     * Returns the root term.
     */
    SearchTerm term() const;

    /*!
     * Sets the maximum number of results.
     *
     * Note that this limit is only evaluated per search backend,
     * so the total number of results retrieved may be larger.
     */
    void setLimit(int limit);

    /*!
     * Returns the maximum number of results.
     *
     * The default value is -1, indicating no limit.
     */
    int limit() const;

    QByteArray toJSON() const;
    static SearchQuery fromJSON(const QByteArray &json);

private:
    QSharedDataPointer<SearchQueryPrivate> d;
};

/*!
 * A search term for an email field.
 *
 * \class Akonadi::EmailSearchTerm
 * \inheaderfile Akonadi/SearchQuery
 * \inmodule AkonadiCore
 *
 * This class can be used to create queries that akonadi email search backends understand.
 *
 * \since 4.13
 */
class AKONADICORE_EXPORT EmailSearchTerm : public SearchTerm
{
public:
    /*!
     * All fields expect a search string unless noted otherwise.
     */
    enum EmailSearchField {
        Subject = 100,      ///< Search in the subject field (expects QString)
        Body,               ///< Search in the email body (expects QString)
        Headers,            ///< @deprecated Will be removed
        HeaderFrom,         ///< Search in the From header (expects QString)
        HeaderTo,           ///< Search in the To header (expects QString)
        HeaderCC,           ///< Search in the CC header (expects QString)
        HeaderBCC,          ///< Search in the BCC header (expects QString)
        HeaderReplyTo,      ///< Search in the ReplyTo header (expects QString)
        HeaderOrganization, ///< Search in the Organization header (expects QString)
        HeaderListId,       ///< Search in the ListId header (expects QString)
        HeaderResentFrom,   ///< @deprecated Will be removed
        HeaderXLoop,        ///< @deprecated Will be removed
        HeaderXMailingList, ///< @deprecated Will be removed
        HeaderXSpamFlag,    ///< @deprecated Will be removed
        HeaderDate,         ///< Match by the Date header (expects QDateTime)
        HeaderOnlyDate,     ///< Match by the Date header, but only by date (expects QDate)
        MessageStatus,      ///< Match by message flags (expects Akonadi::MessageFlag), boolean filter.
        ByteSize,           ///< Match by message size (expects integer)
        AttachmentName,     ///< Search in attachment names (expects QString)
        Attachment,         ///< Search in bodies of plaintext attachments (expects QString)
        Message,            ///< Search in all the QString-based fields listed above
        MessageTag          ///< @deprecated Will be removed
    };

    /*!
     * Constructs an email end term
     */
    EmailSearchTerm(EmailSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /*!
     * Translates field to key
     */
    static QString toKey(EmailSearchField);

    /*!
     * Translates key to field
     */
    static EmailSearchField fromKey(const QString &key);
};

/*!
 * A search term for a contact field.
 *
 * This class can be used to create queries that akonadi contact search backends understand.
 *
 * \class Akonadi::ContactSearchTerm
 * \inheaderfile Akonadi/SearchQuery
 * \inmodule AkonadiCore
 *
 * \since 4.13
 */
class AKONADICORE_EXPORT ContactSearchTerm : public SearchTerm
{
public:
    enum ContactSearchField {
        Name = 200,         ///< Search by full name (expects QString)
        Email,              ///< Search by email address (expects QString)
        Nickname,           ///< Search by nickname (expects QString)
        Uid,                ///< Search by vCard UID (expects QString)
        Birthday,           ///< Match by birthday (expects QDate)
        Anniversary,        ///< Match by anniversary (expects QDate)
        All                 ///< Matches all contacts regardless of value
    };

    ContactSearchTerm(ContactSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /*!
     * Translates field to key
     */
    static QString toKey(ContactSearchField);

    /*!
     * Translates key to field
     */
    static ContactSearchField fromKey(const QString &key);
};

/*!
 * A search term for a incidence field.
 *
 * This class can be used to create queries that akonadi incidence search backends understand.
 *
 * \class Akonadi::IncidenceSearchTerm
 * \inheaderfile Akonadi/SearchQuery
 * \inmodule AkonadiCore
 *
 * \since 5.0
 */
class AKONADICORE_EXPORT IncidenceSearchTerm : public SearchTerm
{
public:
    enum IncidenceSearchField {
        All = 300,          ///< Matches all events regardless of value
        PartStatus,         ///< Match events based on participant status
        Organizer,          ///< Search by incidence organizer name or email
        Summary,            ///< Search by incidence summary
        Location            ///< Search by incidence location
    };

    IncidenceSearchTerm(IncidenceSearchField field, const QVariant &value, SearchTerm::Condition condition = SearchTerm::CondEqual);

    /*!
     * Translates field to key
     */
    static QString toKey(IncidenceSearchField);

    /*!
     * Translates key to field
     */
    static IncidenceSearchField fromKey(const QString &key);
};

}
