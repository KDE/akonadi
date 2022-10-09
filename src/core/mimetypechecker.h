/*
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QSharedDataPointer>

class QString;
#include <QStringList>

namespace Akonadi
{
class Collection;
class Item;
class MimeTypeCheckerPrivate;

/**
 * @short Helper for checking MIME types of Collections and Items.
 *
 * When it is necessary to decide whether an item has a certain MIME type
 * or whether a collection can contain a certain MIME type, direct string
 * comparison might not render the desired result because MIME types can
 * have aliases and be a node in an "inheritance" hierarchy.
 *
 * For example a check like this
 * @code
 * if ( item.mimeType() == QLatin1String( "text/directory" ) )
 * @endcode
 * would fail to detect @c "text/x-vcard" as being the same MIME type.
 *
 * @note KDE deals with this inside the KMimeType framework, this class is just
 * a convenience helper for common Akonadi related checks.
 *
 * Example: Checking whether an Akonadi::Item is contact MIME type
 * @code
 * Akonadi::MimeTypeChecker checker;
 * checker.addWantedMimeType( KContacts::Addressee::mimeType() );
 *
 * if ( checker.isWantedItem( item ) ){
 *   // item.mimeType() is equal KContacts::Addressee::mimeType(), an aliases
 *   // or a sub type.
 * }
 * @endcode
 *
 * Example: Checking whether an Akonadi::Collection could contain calendar
 * items
 * @code
 * Akonadi::MimeTypeChecker checker;
 * checker.addWantedMimeType( QLatin1String( "text/calendar" ) );
 *
 * if ( checker.isWantedCollection( collection ) ) {
 *   // collection.contentMimeTypes() contains @c "text/calendar"
 *   // or a sub type.
 * }
 * @endcode
 *
 * Example: Checking whether an Akonadi::Collection could contain
 * Calendar Event items (i.e. KCal::Event), making use of the respective
 * MIME type "subclassing" provided by Akonadi's MIME type extensions.
 * @code
 * Akonadi::MimeTypeChecker checker;
 * checker.addWantedMimeType( QLatin1String( "application/x-vnd.akonadi.calendar.event" ) );
 *
 * if ( checker.isWantedCollection( collection ) ) {
 *   // collection.contentMimeTypes() contains @c "application/x-vnd.akonadi.calendar.event"
 *   // or a sub type, but just containing @c "text/calendar" would not
 *   // get here
 * }
 * @endcode
 *
 * Example: Checking for items of more than one MIME type and treat one
 * of them specially.
 * @code
 * Akonadi::MimeTypeChecker mimeFilter;
 * mimeFilter.setWantedMimeTypes( QStringList() << KContacts::Addressee::mimeType()
 *                                << KContacts::ContactGroup::mimeType() );
 *
 * if ( mimeFilter.isWantedItem( item ) ) {
 *   if ( Akonadi::MimeTypeChecker::isWantedItem( item, KContacts::ContactGroup::mimeType() ) {
 *     // treat contact group's differently
 *   }
 * }
 * @endcode
 *
 * This class is implicitly shared.
 *
 * @author Kevin Krammer <kevin.krammer@gmx.at>
 *
 * @since 4.3
 */
class AKONADICORE_EXPORT MimeTypeChecker
{
public:
    /**
     * Creates an empty MIME type checker.
     *
     * An empty checker will not report any items or collections as wanted.
     */
    MimeTypeChecker();

    /**
     * Creates a new MIME type checker from an @p other.
     */
    MimeTypeChecker(const MimeTypeChecker &other);

    /**
     * Destroys the MIME type checker.
     */
    ~MimeTypeChecker();

    /**
     * Assigns the @p other to this checker and returns a reference to this checker.
     */
    MimeTypeChecker &operator=(const MimeTypeChecker &other);

    /**
     * Returns the list of wanted MIME types this instance checks against.
     *
     * @note Don't use this just to check whether there are any wanted mimetypes.
     * It is much faster to call @c hasWantedMimeTypes() instead for that purpose.
     *
     * @see setWantedMimeTypes(), hasWantedMimeTypes()
     */
    Q_REQUIRED_RESULT QStringList wantedMimeTypes() const;

    /**
     * Checks whether any wanted MIME types are set.
     *
     * @return @c true if any wanted MIME types are set, false otherwise.
     *
     * @since 5.6.43
     */
    Q_REQUIRED_RESULT bool hasWantedMimeTypes() const;

    /**
     * Sets the list of wanted MIME types this instance checks against.
     *
     * @param mimeTypes The list of MIME types to check against.
     *
     * @see wantedMimeTypes()
     */
    void setWantedMimeTypes(const QStringList &mimeTypes);

    /**
     * Adds another MIME type to the list of wanted MIME types this instance checks against.
     *
     * @param mimeType The MIME types to add to the checklist.
     *
     * @see setWantedMimeTypes()
     */
    void addWantedMimeType(const QString &mimeType);

    /**
     * Removes a MIME type from the list of wanted MIME types this instance checks against.
     *
     * @param mimeType The MIME type to remove from the checklist.
     *
     * @see addWantedMimeType()
     */
    void removeWantedMimeType(const QString &mimeType);

    /**
     * Checks whether a given @p item has one of the wanted MIME types
     *
     * @param item The item to check the MIME type of.
     *
     * @return @c true if the @p item MIME type is one of the wanted ones,
     *         @c false if it isn't, the item is invalid or has an empty MIME type.
     *
     * @see setWantedMimeTypes()
     * @see Item::mimeType()
     */
    Q_REQUIRED_RESULT bool isWantedItem(const Item &item) const;

    /**
     * Checks whether a given @p collection has one of the wanted MIME types
     *
     * @param collection The collection to check the content MIME types of.
     *
     * @return @c true if one of the @p collection content MIME types is
     *         one of the wanted ones, @c false if non is, the collection
     *         is invalid or has an empty content MIME type list.
     *
     * @see setWantedMimeTypes()
     * @see Collection::contentMimeTypes()
     */
    Q_REQUIRED_RESULT bool isWantedCollection(const Collection &collection) const;

    /**
     * Checks whether a given mime type is covered by one of the wanted MIME types.
     *
     * @param mimeType The mime type to check.
     *
     * @return @c true if the mime type @p mimeType is coverd by one of the
     *         wanted MIME types, @c false otherwise.
     *
     * @since 4.6
     */
    Q_REQUIRED_RESULT bool isWantedMimeType(const QString &mimeType) const;

    /**
     * Checks whether any of the given MIME types is covered by one of the wanted MIME types.
     *
     * @param mimeTypes The MIME types to check.
     *
     * @return @c true if any of the MIME types in @p mimeTypes is coverd by one of the
     *         wanted MIME types, @c false otherwise.
     *
     * @since 4.6
     */
    Q_REQUIRED_RESULT bool containsWantedMimeType(const QStringList &mimeTypes) const;

    /**
     * Checks whether a given @p item has the given wanted MIME type
     *
     * @param item The item to check the MIME type of.
     * @param wantedMimeType The MIME type to check against.
     *
     * @return @c true if the @p item MIME type is the given one,
     *         @c false if it isn't, the item is invalid or has an empty MIME type.
     *
     * @see setWantedMimeTypes()
     * @see Item::mimeType()
     */
    Q_REQUIRED_RESULT static bool isWantedItem(const Item &item, const QString &wantedMimeType);

    /**
     * Checks whether a given @p collection has the given MIME type
     *
     * @param collection The collection to check the content MIME types of.
     * @param wantedMimeType The MIME type to check against.
     *
     * @return @c true if one of the @p collection content MIME types is
     *         the given wanted one, @c false if it isn't, the collection
     *         is invalid or has an empty content MIME type list.
     *
     * @see setWantedMimeTypes()
     * @see Collection::contentMimeTypes()
     */
    Q_REQUIRED_RESULT static bool isWantedCollection(const Collection &collection, const QString &wantedMimeType);

private:
    /// @cond PRIVATE
    QSharedDataPointer<MimeTypeCheckerPrivate> d;
    /// @endcond
};

}
