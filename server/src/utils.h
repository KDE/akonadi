/*
 * Copyright (C) 2010 Tobias Koenig <tokoe@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include <QtCore/QVariant>

namespace Utils {

/**
 * Converts a QVariant to a QString depending on its internal type.
 */
static inline QString variantToString( const QVariant &variant )
{
  if ( variant.type() == QVariant::String )
    return variant.toString();
  else if ( variant.type() == QVariant::ByteArray )
    return QString::fromUtf8( variant.toByteArray() );
  else {
    qWarning( "Unable to convert variant of type %s to QString", variant.typeName() );
    Q_ASSERT( false );
    return QString();
  }
}

/**
 * Converts a QVariant to a QByteArray depending on its internal type.
 */
static inline QByteArray variantToByteArray( const QVariant &variant )
{
  if ( variant.type() == QVariant::String )
    return variant.toString().toUtf8();
  else if ( variant.type() == QVariant::ByteArray )
    return variant.toByteArray();
  else {
    qWarning( "Unable to convert variant of type %s to QByteArray", variant.typeName() );
    Q_ASSERT( false );
    return QByteArray();
  }
}

}

#endif
