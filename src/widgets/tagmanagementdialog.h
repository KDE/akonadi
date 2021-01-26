/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGMANAGEMENTDIALOG_H
#define AKONADI_TAGMANAGEMENTDIALOG_H

#include "akonadiwidgets_export.h"

#include "tag.h"
#include <QDialog>
class QDialogButtonBox;
namespace Akonadi
{
/**
 * A dialog to manage tags.
 *
 * @since 4.13
 */
class AKONADIWIDGETS_EXPORT TagManagementDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagManagementDialog(QWidget *parent = nullptr);
    ~TagManagementDialog() override;

    Q_REQUIRED_RESULT QDialogButtonBox *buttons() const;

private:
    struct Private;
    QScopedPointer<Private> d;
};

}

#endif
