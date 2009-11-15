/*
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dbusoperators.h"
#include "querymetatype.h"

#include <soprano/version.h>

#include <QtDBus/QDBusMetaType>


Q_DECLARE_METATYPE(Nepomuk::Search::Result)
Q_DECLARE_METATYPE(Soprano::Node)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<Nepomuk::Search::Result>)


void Nepomuk::Search::registerDBusTypes()
{
    qDBusRegisterMetaType<Nepomuk::Search::Result>();
    qDBusRegisterMetaType<QList<Nepomuk::Search::Result> >();
    qDBusRegisterMetaType<Soprano::Node>();
    qDBusRegisterMetaType<RequestPropertyMapDBus>();
}


QDBusArgument& operator<<( QDBusArgument& arg, const Nepomuk::Search::Result& result )
{
    //
    // Signature: (sda{s(isss)})
    //

    arg.beginStructure();

    arg << QString::fromAscii( result.resourceUri().toEncoded() ) << result.score();

    arg.beginMap( QVariant::String, qMetaTypeId<Soprano::Node>() );

    QHash<QUrl, Soprano::Node> rp = result.requestProperties();
    for ( QHash<QUrl, Soprano::Node>::const_iterator it = rp.constBegin(); it != rp.constEnd(); ++it ) {
        arg.beginMapEntry();
        arg << QString::fromAscii( it.key().toEncoded() ) << it.value();
        arg.endMapEntry();
    }

    arg.endMap();

    arg.endStructure();

    return arg;
}


const QDBusArgument& operator>>( const QDBusArgument& arg, Nepomuk::Search::Result& result )
{
    //
    // Signature: (sda{s(isss)})
    //

    arg.beginStructure();
    QString uri;
    double score = 0.0;

    arg >> uri >> score;
    result = Nepomuk::Search::Result( QUrl::fromEncoded( uri.toAscii() ), score );

    arg.beginMap();
    while ( !arg.atEnd() ) {
        QString rs;
        Soprano::Node node;
        arg.beginMapEntry();
        arg >> rs >> node;
        arg.endMapEntry();
        result.addRequestProperty( QUrl::fromEncoded( rs.toAscii() ), node );
    }
    arg.endMap();

    arg.endStructure();

    return arg;
}


QDBusArgument& operator<<( QDBusArgument& arg, const Soprano::Node& node )
{
    arg.beginStructure();
    arg << ( int )node.type();
    if ( node.type() == Soprano::Node::ResourceNode ) {
        arg << QString::fromAscii( node.uri().toEncoded() );
    }
    else {
        arg << node.toString();
    }
    arg << node.language() << node.dataType().toString();
    arg.endStructure();
    return arg;
}


const QDBusArgument& operator>>( const QDBusArgument& arg, Soprano::Node& node )
{
    //
    // Signature: (isss)
    //
    arg.beginStructure();
    int type;
    QString value, language, dataTypeUri;
    arg >> type >> value >> language >> dataTypeUri;
    if ( type == Soprano::Node::LiteralNode ) {
#if SOPRANO_IS_VERSION( 2, 3, 0 )
        if ( dataTypeUri.isEmpty() )
            node = Soprano::Node( Soprano::LiteralValue::createPlainLiteral( value, language ) );
        else
            node = Soprano::Node( Soprano::LiteralValue::fromString( value, QUrl::fromEncoded( dataTypeUri.toAscii() ) ) );
#else
        node = Soprano::Node( Soprano::LiteralValue::fromString( value, dataTypeUri ), language );
#endif
    }
    else if ( type == Soprano::Node::ResourceNode ) {
        node = Soprano::Node( QUrl::fromEncoded( value.toAscii() ) );
    }
    else if ( type == Soprano::Node::BlankNode ) {
        node = Soprano::Node( value );
    }
    else {
        node = Soprano::Node();
    }
    arg.endStructure();
    return arg;
}
