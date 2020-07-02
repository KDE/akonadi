/*
  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
  SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADI_TAGSELECTIONCOMBO_H
#define AKONADI_TAGSELECTIONCOMBO_H

#include "akonadiwidgets_export.h"

#include "tag.h"

#include <QComboBox>

#include <memory>

namespace Akonadi {
/**
 * @brief The TagSelectionCombo class
 */
class AKONADIWIDGETS_EXPORT TagSelectionComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit TagSelectionComboBox(QWidget *parent = nullptr);
    ~TagSelectionComboBox() override;

    void setCheckable(bool checkable);
    bool checkable() const;

    Tag::List selection() const;
    QStringList selectionNames() const;
    void setSelection(const Tag::List &selection);
    void setSelection(const QStringList &selection);

    void hidePopup() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *receiver, QEvent *event) override;

Q_SIGNALS:
    void selectionChanged(const Akonadi::Tag::List &selection);

private:
    class Private;
    std::unique_ptr<Private> const d;
};

} // namespace

#endif // TAGSELECTIONCOMBO_H
