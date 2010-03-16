/*
  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef PUBLIC_ETM_H
#define PUBLIC_ETM_H

#include <akonadi/changerecorder.h>
#include <akonadi/entitytreemodel.h>
#include "entitytreemodel_p.h"

using namespace Akonadi;

class PublicETMPrivate;

class PublicETM : public EntityTreeModel
{
  Q_OBJECT
  Q_DECLARE_PRIVATE(PublicETM)
public:
  PublicETM( ChangeRecorder *monitor, QObject *parent );

  EntityTreeModelPrivate *privateClass() const { return d_ptr; }
};

class PublicETMPrivate : public EntityTreeModelPrivate
{
  Q_DECLARE_PUBLIC(PublicETM)

public:
  PublicETMPrivate( PublicETM *p );
};

#endif
