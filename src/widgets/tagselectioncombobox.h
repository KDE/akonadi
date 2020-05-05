/*
  Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
  Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
