/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

namespace Akonadi
{
/**
 * @short An interface to report differences between two arbitrary objects.
 *
 * This interface can be used to report differences between two arbitrary objects
 * by describing a virtual table with three columns. The first column contains the name
 * of the property that is compared, the second column the property value of the one
 * object and the third column the property of the other object.
 *
 * The rows of this table can have different modes:
 *   @li NormalMode The left and right columns show the same property values.
 *   @li ConflictMode The left and right columns show conflicting property values.
 *   @li AdditionalLeftMode The left column contains a property value that is not available in the right column.
 *   @li AdditionalRightMode The right column contains a property value that is not available in the left column.
 *
 * Example:
 *
 * @code
 * // add differences of a contact
 * const KContacts::Addressee contact1 = ...
 * const KContacts::Addressee contact2 = ...
 *
 * AbstractDifferencesReporter *reporter = ...
 * reporter->setPropertyNameTitle( i18n( "Contact fields" ) );
 * reporter->setLeftPropertyValueTitle( i18n( "Changed Contact" ) );
 * reporter->setRightPropertyValueTitle( i18n( "Conflicting Contact" ) );
 *
 * // check given name
 * if ( contact1.givenName() != contact2.givenName() )
 *   reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Given Name" ),
 *                          contact1.givenName(), contact2.givenName() );
 * else
 *   reporter->addProperty( AbstractDifferencesReporter::NormalMode, i18n( "Given Name" ),
 *                          contact1.givenName(), contact2.givenName() );
 *
 * // check family name
 * if ( contact1.familyName() != contact2.familyName() )
 *   reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Family Name" ),
 *                          contact1.givenName(), contact2.givenName() );
 * else
 *   reporter->addProperty( AbstractDifferencesReporter::NormalMode, i18n( "Family Name" ),
 *                          contact1.givenName(), contact2.givenName() );
 *
 * // check emails
 * const QStringList leftEmails = contact1.emails();
 * const QStringList rightEmails = contact2.emails();
 *
 * for ( const QString &leftEmail : leftEmails ) {
 *   if ( rightEmails.contains( leftEmail ) )
 *     reporter->addProperty( AbstractDifferencesReporter::NormalMode, i18n( "Email" ),
 *                            leftEmail, leftEmail );
 *   else
 *     reporter->addProperty( AbstractDifferencesReporter::AdditionalLeftMode, i18n( "Email" ),
 *                            leftEmail, QString() );
 * }
 *
 * for( const QString &rightEmail : rightEmails ) {
 *   if ( !leftEmails.contains( rightEmail ) )
 *     reporter->addProperty( AbstractDifferencesReporter::AdditionalRightMode, i18n( "Email" ),
 *                            QString(), rightEmail );
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class AbstractDifferencesReporter
{
public:
    /**
     * Describes the property modes.
     */
    enum Mode {
        NormalMode, ///< The left and right column show the same property values.
        ConflictMode, ///< The left and right column show conflicting property values.
        AdditionalLeftMode, ///< The left column contains a property value that is not available in the right column.
        AdditionalRightMode ///< The right column contains a property value that is not available in the left column.
    };

    /**
     * Destroys the abstract differences reporter.
     */
    virtual ~AbstractDifferencesReporter() = default;

    /**
     * Sets the @p title of the property name column.
     */
    virtual void setPropertyNameTitle(const QString &title) = 0;

    /**
     * Sets the @p title of the column that shows the property values
     * of the left object.
     */
    virtual void setLeftPropertyValueTitle(const QString &title) = 0;

    /**
     * Sets the @p title of the column that shows the property values
     * of the right object.
     */
    virtual void setRightPropertyValueTitle(const QString &title) = 0;

    /**
     * Adds a new property entry to the table.
     *
     * @param mode Describes the mode of the property. If mode is AdditionalLeftMode or AdditionalRightMode, rightValue resp. leftValue
     *             should be QString().
     * @param name The user visible name of the property.
     * @param leftValue The user visible property value of the left object.
     * @param rightValue The user visible property value of the right object.
     */
    virtual void addProperty(Mode mode, const QString &name, const QString &leftValue, const QString &rightValue) = 0;

protected:
    explicit AbstractDifferencesReporter() = default;

private:
    Q_DISABLE_COPY_MOVE(AbstractDifferencesReporter)
};

}

