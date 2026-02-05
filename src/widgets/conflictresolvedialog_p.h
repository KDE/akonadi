/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>

#include "akonadiwidgetstests_export.h"
#include "conflicthandler_p.h"

class QTextBrowser;

namespace Akonadi
{
/**
 * @short A dialog to ask the user for a resolve strategy for conflicts.
 *
 * \author Tobias Koenig <tokoe@kde.org>
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

    ~ConflictResolveDialog() override;

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
    [[nodiscard]] ConflictHandler::ResolveStrategy resolveStrategy() const;

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
