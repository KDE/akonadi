/*
    This file is part of Akonadi

    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGEDITWIDGET_P_H
#define AKONADI_TAGEDITWIDGET_P_H

#include <QWidget>
#include "tag.h"
#include "akonadiwidgets_export.h"

namespace Akonadi
{

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
    explicit TagEditWidget(QWidget *parent = nullptr);
    explicit TagEditWidget(Akonadi::TagModel *model, QWidget *parent = nullptr, bool enableSelection = false);
    ~TagEditWidget() override;

    void setModel(Akonadi::TagModel *model);
    Akonadi::TagModel *model() const;

    void setSelectionEnabled(bool enabled);
    bool selectionEnabled() const;

    void setSelection(const Akonadi::Tag::List &tags);
    Q_REQUIRED_RESULT Akonadi::Tag::List selection() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}

#endif
