/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SUBSCRIPTIONDIALOG_H
#define AKONADI_SUBSCRIPTIONDIALOG_H

#include "akonadiwidgets_export.h"

#include <QDialog>

namespace Akonadi {

/**
 * Local subscription dialog.
 */
class AKONADIWIDGETS_EXPORT SubscriptionDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Creates a new subscription dialog.
     *
     * @param parent The parent widget.
     */
    explicit SubscriptionDialog(QWidget *parent = Q_NULLPTR);

    /**
     * Creates a new subscription dialog.
     *
     * @param parent The parent widget.
     * @param mimetypes The specific mimetypes
     * @since 4.6
     */
    explicit SubscriptionDialog(const QStringList &mimetypes, QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the subscription dialog.
     *
     * @note Don't call the destructor manually, the dialog will
     *       be destructed automatically as soon as all changes
     *       are written back to the server.
     */
    ~SubscriptionDialog();

    /**
     * @param showHidden shows hidden collections if set as @c true
     * @since 4.9
     */
    void showHiddenCollection(bool showHidden);

private:
    class Private;
    Private *const d;

    void init(const QStringList &mimetypes);

    Q_PRIVATE_SLOT(d, void done())
    Q_PRIVATE_SLOT(d, void subscriptionResult(KJob *job))
    Q_PRIVATE_SLOT(d, void modelLoaded())
    Q_PRIVATE_SLOT(d, void slotSetPattern(const QString &))
    Q_PRIVATE_SLOT(d, void slotSetIncludeCheckedOnly(bool))
    Q_PRIVATE_SLOT(d, void slotUnSubscribe())
    Q_PRIVATE_SLOT(d, void slotSubscribe())
};

}

#endif
