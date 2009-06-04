/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_TRANSPORTRESOURCE_H
#define AKONADI_TRANSPORTRESOURCE_H

#include "akonadi_export.h"

#include <QString>

#include <akonadi/item.h>
#include <akonadi/resourcebase.h>

class KJob;

namespace Akonadi {

/**
  * @short Resource implementing mail transport capability.
  *
  * TODO docu
 */
// TODO name: TransportResourceBase?
class AKONADI_EXPORT TransportResource : public ResourceBase
{
  Q_OBJECT

  public:
#if 0
    /**
     * Use this method in the main function of your resource
     * application to initialize your resource subclass.
     * This method also takes care of creating a KApplication
     * object and parsing command line arguments.
     *
     * @note In case the given class is also derived from AgentBase::Observer
     *       it gets registered as its own observer (see AgentBase::Observer), e.g.
     *       <tt>resourceInstance->registerObserver( resourceInstance );</tt>
     *
     * @code
     *
     *   class MyResource : public ResourceBase
     *   {
     *     ...
     *   };
     *
     *   int main( int argc, char **argv )
     *   {
     *     return ResourceBase::init<MyResource>( argc, argv );
     *   }
     *
     * @endcode
     */
    template <typename T>
    static int init( int argc, char **argv )
    {
      const QString id = parseArguments( argc, argv );
      KApplication app;
      T* r = new T( id );

      // check if T also inherits AgentBase::Observer and
      // if it does, automatically register it on itself
      Observer *observer = dynamic_cast<Observer*>( r );
      if ( observer != 0 )
        r->registerObserver( observer );
      return init( r );
    }
#endif

  Q_SIGNALS:
    void transportResult( bool result, const QString &message );

  public Q_SLOTS:
    virtual void send( Akonadi::Item::Id message ) = 0;

  protected:
    TransportResource( const QString &id );

  protected Q_SLOTS:
    void emitResult( KJob* );

  private:
    // TODO probably need a private class.

};

}

#if 0
#ifndef AKONADI_RESOURCE_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi resources.
 */
#define AKONADI_RESOURCE_MAIN( resourceClass )                       \
  int main( int argc, char **argv )                            \
  {                                                            \
    return Akonadi::ResourceBase::init<resourceClass>( argc, argv ); \
  }
#endif
#endif

#endif
