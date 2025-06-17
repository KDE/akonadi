// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <QWidget>

class KnutSettings;
class QLineEdit;
class QCheckBox;

class AccountWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountWidget(KnutSettings &settings, const QString &identifier, QWidget *parent);

    void loadSettings();
    void saveSettings();

Q_SIGNALS:
    void okEnabled(bool enabled);

private:
    const QString mIdentifier;
    KnutSettings &mSettings;
    QLineEdit *const mLineEdit;
    QCheckBox *const mFileWatchingCheckBox;
};
