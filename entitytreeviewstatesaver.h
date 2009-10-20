/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ENTITYTREEVIEWSTATESAVER_H
#define AKONADI_ENTITYTREEVIEWSTATESAVER_H

#include "akonadi_export.h"

#include <QtCore/QObject>

class QTreeView;
class KConfigGroup;

namespace Akonadi {

class EntityTreeViewStateSaverPrivate;

/**
 * @short A helper class that saves and restores state of an EntityTreeView
 *
 * This class saves and restores the state of a QTreeView showing an EntityTreeModel,
 * or whatever proxies are stacked on top of one. State so far means the current selection
 * and expansion of the items and collections.
 *
 * The EntityTreeViewStateSaver is needed because populating the EntityTreeModel
 * is asynchronous and finding the right point in time to restore the state is a
 * difficult task that is hidden inside this class.
 *
 * Example:
 *
 * @code
 *
 * class MyWidget : public QWidget
 * {
 *   public:
 *      MyWidget( QWidget* parent );
 *      ~MyWidget();
 *
 *      ...
 *
 *   private:
 *     KConfig *mGlobalConfig;
 *     QTreeView *mTreeView;
 *     Akonadi::EntityTreeViewStateSaver *mStateSaver;
 * };
 *
 * MyWidget::MyWidget( QWidget *parent )
 *   : QWidget( parent )
 * {
 *   mGlobalConfig = ...
 *   mTreeView = new QTreeView( this );
 *   ...
 *
 *   mStateSaver = new Akonadi::EntityTreeViewStateSaver( mTreeView );
 *
 *   KConfigGroup group( mGlobalConfig, "treeViewState" );
 *   mStateSaver->restoreState( group );
 * }
 *
 * MyWidget::~MyWidget()
 * {
 *   KConfigGroup group( mGlobalConfig, "treeViewState" );
 *   mStateSaver->saveState( group );
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @since 4.4
 */
class AKONADI_EXPORT EntityTreeViewStateSaver : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new state saver, for saving or restoring.
     *
     * @param view The QTreeView which state should be saved/restored.
     */
    explicit EntityTreeViewStateSaver( QTreeView* view );

    /**
     * Destroys this state saver.
     */
    ~EntityTreeViewStateSaver();

    /**
     * Stores the current state in the given config group.
     *
     * @param configGroup Config file group into which the state is supposed to be stored.
     */
    void saveState( KConfigGroup &configGroup ) const;

    /**
     * Restore the state stored in @p configGroup as soon as the corresponding entities
     * become available in the model (the model is populated asynchronously, which is the
     * main reason why you want to use this class).
     *
     * @param configGroup Config file group containing a previously stored EntityTreeView state.
     */
    void restoreState( const KConfigGroup &configGroup ) const;

  private:
    //@cond PRIVATE
    EntityTreeViewStateSaverPrivate* const d;
    Q_PRIVATE_SLOT( d, void rowsInserted( const QModelIndex&, int, int ) )
    Q_PRIVATE_SLOT( d, void restoreScrollBarState() )
    //@endcond
};

}

#endif
