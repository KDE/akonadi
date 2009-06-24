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

#ifndef AKONADI_FILTERACTIONJOB_H
#define AKONADI_FILTERACTIONJOB_H

#include "item.h"
#include "transactionsequence.h"

namespace Akonadi {

class Collection;
class ItemFetchScope;
class Job;

/**
  Abstract functor for a FilterActionJob.  Subclass it and implement the virtual
  methods.

  // TODO docu

  // TODO example
*/
class AKONADI_EXPORT FilterAction
{
  public:
    /// only used if FilterActionJob created with collection constructor
    virtual Akonadi::ItemFetchScope fetchScope() const = 0;
    virtual bool itemAccepted( const Akonadi::Item &item ) const = 0;
    virtual Akonadi::Job *itemAction( const Akonadi::Item &item ) const = 0;
    virtual ~FilterAction();
};


/**
  A job to filter and apply an action on a set of items.  The filter and action
  are provided by a functor derived from FilterAction.

  // TODO docu

  // TODO example
*/
class AKONADI_EXPORT FilterActionJob : public TransactionSequence
{
  Q_OBJECT

  public:
    FilterActionJob( const Item &item, FilterAction *functor, QObject *parent = 0 );
    FilterActionJob( const Item::List &items, FilterAction *functor, QObject *parent = 0 );
    FilterActionJob( const Collection &collection, FilterAction *functor, QObject *parent = 0 );
    ~FilterActionJob();

    // TODO I would like to provide a list of modified items, but there is no
    // easy way to get those, because FilterAction::itemAction() can return any
    // kind of Job, not only an ItemModifyJob.
    // Restrict it to modify jobs only?
    // Re-fetch the accepted items after all jobs are done?
    //Item::List items() const;

    // TODO provide setFunctor?

    // TODO provide forceFetch for the Item and Item::List constructors?

  protected:
    virtual void doStart();

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void fetchResult( KJob* ) )
    //@endcond
};

}


#endif

