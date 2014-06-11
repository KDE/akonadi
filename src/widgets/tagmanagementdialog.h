/*
    This file is part of Akonadi

    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_TAGMANAGEMENTDIALOG_H
#define AKONADI_TAGMANAGEMENTDIALOG_H

#include "akonadiwidgets_export.h"

#include <QDialog>
#include "tag.h"

namespace Akonadi {

/**
 * A dialog to manage tags.
 *
 * @since 4.13
 */
class AKONADIWIDGETS_EXPORT TagManagementDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagManagementDialog(QWidget *parent = 0);
    virtual ~TagManagementDialog();

private:
    class Private;
    QSharedPointer<Private> d;
};

}

#endif
