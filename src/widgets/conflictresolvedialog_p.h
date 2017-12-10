/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONFLICTRESOLVEDIALOG_P_H
#define AKONADI_CONFLICTRESOLVEDIALOG_P_H

#include <QDialog>

#include "conflicthandler_p.h"
#include "akonadiwidgetstests_export.h"

class QTextBrowser;

namespace Akonadi
{

/**
 * @short A dialog to ask the user for a resolve strategy for conflicts.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGET_TESTS_EXPORT ConflictResolveDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Creates a new conflict resolve dialog.
     *
     * @param parent The parent widget.
     */
    explicit ConflictResolveDialog(QWidget *parent = nullptr);

    ~ConflictResolveDialog();

    /**
     * Sets the items that causes the conflict.
     *
     * @param localItem The local item which causes the conflict.
     * @param otherItem The conflicting item from the Akonadi storage.
     *
     * @note Both items need the full payload set.
     */
    void setConflictingItems(const Akonadi::Item &localItem, const Akonadi::Item &otherItem);

    /**
     * Returns the resolve strategy the user choose.
     */
    ConflictHandler::ResolveStrategy resolveStrategy() const;

private Q_SLOTS:
    void slotUseLocalItemChoosen();
    void slotUseOtherItemChoosen();
    void slotUseBothItemsChoosen();
    void slotOpenEditor();

private:
    ConflictHandler::ResolveStrategy mResolveStrategy;

    Akonadi::Item mLocalItem;
    Akonadi::Item mOtherItem;

    QTextBrowser *mView = nullptr;
    QString mTextContent;
};

}

#endif
