/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef CUSTOMFIELDS_P_H
#define CUSTOMFIELDS_P_H

#include <QtCore/QString>
#include <QtCore/QVariant>

template <typename T>
class QVector;

/**
 * @short A class that represents non-standard contact fields.
 *
 * There exists three scopes of fields. To the local scope belong all
 * custom fields that are defined by the user and that exists only for one
 * contact. The description for these fields are stored inside ContactMetaData
 * as custom attribute of the Akonadi item that represents the contact.
 * To the global scope belong all custom fields that are defined by the user but
 * shall be available in all contacts of the address book. Their description
 * is stored by CustomFieldManager in $HOME/.kde/share/config/akonadi_contactrc.
 * All other custom fields belong to the external scope, they come with import
 * of contacts from other PIM applications (e.g. further X- entries in vCards).
 * Their description is created on the fly when editing the custom fields.
 *
 * The description of a custom field covers the key, title and type.
 */
class CustomField
{
  public:
    typedef QVector<CustomField> List;

    enum Type {
      TextType,
      NumericType,
      BooleanType,
      DateType,
      TimeType,
      DateTimeType,
      UrlType
    };

    enum Scope {
      LocalScope,   ///< Field has been defined by user for one contact
      GlobalScope,  ///< Field has been defined by user for all contacts
      ExternalScope ///< Field has been defined by the external data source (e.g. vCard)
    };

    CustomField();
    CustomField( const QString &key, const QString &title, Type type, Scope scope );

    static CustomField fromVariantMap( const QVariantMap &map, Scope scope );

    void setKey( const QString &key );
    QString key() const;

    void setTitle( const QString &title );
    QString title() const;

    void setType( Type type );
    Type type() const;

    void setScope( Scope scope );
    Scope scope() const;

    void setValue( const QString &value );
    QString value() const;

    QVariantMap toVariantMap() const;

    static QString typeToString( Type type );
    static Type stringToType( const QString &type );

  private:
    QString mKey;
    QString mTitle;
    Type mType;
    Scope mScope;
    QString mValue;
};

#endif
