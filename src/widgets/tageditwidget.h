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

#ifndef AKONADI_TAGEDITWIDGET_P_H
#define AKONADI_TAGEDITWIDGET_P_H

#include <QWidget>
#include "tag.h"
#include "akonadiwidgets_export.h"

namespace Akonadi {

class TagModel;
/**
 * A widget that offers facilities to add/remove tags and optionally provides a way to select tags.
 *
 * @since 4.13
 */
class AKONADIWIDGETS_EXPORT TagEditWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TagEditWidget(Akonadi::TagModel *model, QWidget *parent = Q_NULLPTR, bool enableSelection = false);
    virtual ~TagEditWidget();

    void setSelection(const Akonadi::Tag::List &tags);
    Akonadi::Tag::List selection() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

private:
    class Private;
    QSharedPointer<Private> d;
};

}

#endif
