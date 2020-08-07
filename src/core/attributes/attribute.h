/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ATTRIBUTE_H
#define AKONADI_ATTRIBUTE_H

#include "akonadicore_export.h"

#include <QList>

namespace Akonadi
{

/**
 * @short Provides interface for custom attributes for Entity.
 *
 * This class is an interface for custom attributes, that can be stored
 * in an entity. Attributes should be meta data, e.g. ACLs, quotas etc.
 * that are not part of the entities' data itself.
 *
 * Note that attributes are per user, i.e. when an attribute is added to
 * an entity, it only applies to the current user.
 *
 * To provide custom attributes, you have to subclass from this interface
 * and reimplement the pure virtual methods.
 *
 * @code
 *
 * class SecrecyAttribute : public Akonadi::Attribute
 * {
 *    public:
 *      enum Secrecy
 *      {
 *        Public,
 *        Private,
 *        Confidential
 *      };
 *
 *      SecrecyAttribute( Secrecy secrecy = Public )
 *        : mSecrecy( secrecy )
 *      {
 *      }
 *
 *      void setSecrecy( Secrecy secrecy )
 *      {
 *        mSecrecy = secrecy;
 *      }
 *
 *      Secrecy secrecy() const
 *      {
 *        return mSecrecy;
 *      }
 *
 *      virtual QByteArray type() const
 *      {
 *        return "secrecy";
 *      }
 *
 *      virtual Attribute* clone() const
 *      {
 *        return new SecrecyAttribute( mSecrecy );
 *      }
 *
 *      virtual QByteArray serialized() const
 *      {
 *        switch ( mSecrecy ) {
 *          case Public: return "public"; break;
 *          case Private: return "private"; break;
 *          case Confidential: return "confidential"; break;
 *        }
 *      }
 *
 *      virtual void deserialize( const QByteArray &data )
 *      {
 *        if ( data == "public" )
 *          mSecrecy = Public;
 *        else if ( data == "private" )
 *          mSecrecy = Private;
 *        else if ( data == "confidential" )
 *          mSecrecy = Confidential;
 *      }
 * }
 *
 * @endcode
 *
 * Additionally, you need to register your attribute with Akonadi::AttributeFactory
 * for automatic deserialization during retrieving of collections or items:
 *
 * @code
 * AttributeFactory::registerAttribute<SecrecyAttribute>();
 * @endcode
 *
 * Third party attributes need to be registered once by each application that uses
 * them. So the above snippet needs to be in the resource that adds the attribute,
 * and each application that uses the resource. This may be simplified in the future.
 *
 * The custom attributes can be used in the following way:
 *
 * @code
 *
 * Akonadi::Item item( "text/directory" );
 * SecrecyAttribute* attr = item.attribute<SecrecyAttribute>( Item::AddIfMissing );
 * attr->setSecrecy( SecrecyAttribute::Confidential );
 *
 * @endcode
 *
 * and
 *
 * @code
 *
 * Akonadi::Item item = ...
 *
 * if ( item.hasAttribute<SecrecyAttribute>() ) {
 *   SecrecyAttribute *attr = item.attribute<SecrecyAttribute>();
 *
 *   SecrecyAttribute::Secrecy secrecy = attr->secrecy();
 *   ...
 * }
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT Attribute // clazy:exclude=copyable-polymorphic
{
public:
    /**
     * Describes a list of attributes.
     */
    typedef QList<Attribute *> List;

    /**
     * Returns the type of the attribute.
     */
    virtual QByteArray type() const = 0;

    /**
     * Destroys this attribute.
     */
    virtual ~Attribute();

    /**
     * Creates a copy of this attribute.
     */
    virtual Attribute *clone() const = 0;

    /**
     * Returns a QByteArray representation of the attribute which will be
     * storaged. This can be raw binary data, no encoding needs to be applied.
     */
    virtual QByteArray serialized() const = 0;

    /**
     * Sets the data of this attribute, using the same encoding
     * as returned by toByteArray().
     *
     * @param data The encoded attribute data.
     */
    virtual void deserialize(const QByteArray &data) = 0;

protected:
    explicit Attribute() = default;
    Attribute(const Attribute &) = default;
};

} // namespace Akonadi

#endif
