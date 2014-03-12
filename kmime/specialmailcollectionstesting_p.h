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

#ifndef AKONADI_SPECIALMAILCOLLECTIONSTESTING_P_H
#define AKONADI_SPECIALMAILCOLLECTIONSTESTING_P_H

#include "akonadi-kmimeprivate_export.h"
#include "specialmailcollections.h"

namespace Akonadi {

/**
  @internal
  Class that exposes SpecialMailCollections' private methods for use in unit tests.
  HACK Is there a better way to do this?
*/
class AKONADI_KMIME_TEST_EXPORT SpecialMailCollectionsTesting
{
public:
    static SpecialMailCollectionsTesting *_t_self();
    void _t_setDefaultResourceId(const QString &resourceId);
    void _t_forgetFoldersForResource(const QString &resourceId);
    void _t_beginBatchRegister();
    void _t_endBatchRegister();
    int _t_knownResourceCount() const;
    int _t_knownFolderCount() const;
};

} // namespace Akonadi

#endif // AKONADI_SPECIALMAILCOLLECTIONSTESTING_H
