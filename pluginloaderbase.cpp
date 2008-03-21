/*  -*- c++ -*-
    This file is part of libkdepim.

    Copyright (c) 2002,2004 Marc Mutz <mutz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <pluginloaderbase.h>

#include <kconfiggroup.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <klibloader.h>
#include <kglobal.h>
#include <kdebug.h>

#include <QFile>
#include <QStringList>

namespace KPIM {

  PluginLoaderBase::PluginLoaderBase() : d(0) {}
  PluginLoaderBase::~PluginLoaderBase() {}


  QStringList PluginLoaderBase::types() const {
    QStringList result;
    for ( QMap< QString, PluginMetaData >::const_iterator it = mPluginMap.begin(); it != mPluginMap.end() ; ++it )
      result.push_back( it.key() );
    return result;
  }

  const PluginMetaData * PluginLoaderBase::infoForName( const QString & type ) const {
    return mPluginMap.contains( type ) ? &(const_cast<PluginLoaderBase*>(this)->mPluginMap[type]) : 0 ;
  }


  void PluginLoaderBase::doScan( const char * path ) {
    mPluginMap.clear();

    const QStringList list =
      KGlobal::dirs()->findAllResources( "data", QLatin1String( path ),
                                         KStandardDirs::Recursive |
                                         KStandardDirs::NoDuplicates );
    for ( QStringList::const_iterator it = list.begin(); it != list.end(); ++it ) {
      KConfig config( *it, KConfig::SimpleConfig);
      if ( config.hasGroup( "Misc" ) && config.hasGroup( "Plugin" ) ) {
        KConfigGroup group( &config, "Plugin" );

        const QString type = group.readEntry( "Type" ).toLower();
        if ( type.isEmpty() ) {
          kWarning( 5300 ) << "missing or empty [Plugin]Type value in \"" << *it << "\" - skipping" << endl;
          continue;
        }

        const QString library = group.readEntry( "X-KDE-Library" );
        if ( library.isEmpty() ) {
          kWarning( 5300 ) << "missing or empty [Plugin]X-KDE-Library value in \"" << *it << "\" - skipping" << endl;
          continue;
        }

        KConfigGroup group2( &config, "Misc" );

        QString name = group2.readEntry( "Name" );
        if ( name.isEmpty() ) {
          kWarning( 5300 ) << "missing or empty [Misc]Name value in \"" << *it << "\" - inserting default name" << endl;
          name = i18n("Unnamed plugin");
        }

        QString comment = group2.readEntry( "Comment" );
        if ( comment.isEmpty() ) {
          kWarning( 5300 ) << "missing or empty [Misc]Comment value in \"" << *it << "\" - inserting default name" << endl;
          comment = i18n("No description available");
        }

        mPluginMap.insert( type, PluginMetaData( library, name, comment ) );
      } else {
        kWarning( 5300 ) << "Desktop file \"" << *it << "\" doesn't seem to describe a plugin " << "(misses Misc and/or Plugin group)" << endl;
      }
    }
  }

  KLibrary::void_function_ptr PluginLoaderBase::mainFunc( const QString & type, const char * mf_name ) const {
    if ( type.isEmpty() || !mPluginMap.contains( type ) )
      return 0;

    const QString libName = mPluginMap[ type ].library;
    if ( libName.isEmpty() )
      return 0;

    const KLibrary * lib = openLibrary( libName );
    if ( !lib )
      return 0;

    PluginMetaData pmd = mPluginMap.value( type );
    pmd.loaded = true;
    mPluginMap[ type ] = pmd;

    const QString factory_name = libName + QLatin1Char( '_' ) + QLatin1String( mf_name );
    KLibrary::void_function_ptr sym = const_cast<KLibrary*>( lib )->resolveFunction( factory_name.toLatin1() );
    if ( !sym ) {
      kWarning( 5300 ) << "No symbol named \"" << factory_name.toLatin1() << "\" (" << factory_name << ") was found in library \"" << libName << "\"" << endl;
      return 0;
    }

    return sym;
  }

  const KLibrary * PluginLoaderBase::openLibrary( const QString & libName ) const {

    const QString path = KLibLoader::findLibrary( libName );

    if ( path.isEmpty() ) {
      kWarning( 5300 ) << "No plugin library named \"" << libName << "\" was found!" << endl;
      return 0;
    }

    const KLibrary * library = KLibLoader::self()->library( path );

    kDebug( !library, 5300 ) << "Could not load library '" << libName << "'" << endl;

    return library;
  }


} // namespace KPIM
