// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include "accountwidget.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <KLocalizedString>

#include "settings.h"

using namespace Qt::StringLiterals;

AccountWidget::AccountWidget(KnutSettings &settings, const QString &identifier, QWidget *parent)
    : QWidget(parent)
    , mIdentifier(identifier)
    , mSettings(settings)
    , mLineEdit(new QLineEdit)
    , mFileWatchingCheckBox(new QCheckBox)
{
    auto layout = new QFormLayout(this);

    auto hlay = new QHBoxLayout;
    hlay->addWidget(mLineEdit);

    auto button = new QPushButton(QIcon::fromTheme(u"document-open-folder-symbolic"_s), {});
    hlay->addWidget(button);

    connect(button, &QPushButton::clicked, this, [this] {
        const QString newFile =
            QFileDialog::getSaveFileName(this,
                                         i18n("Select Data File"),
                                         QString(),
                                         QStringLiteral("*.xml |") + i18nc("Filedialog filter for Akonadi data file", "Akonadi Knut Data File"));

        if (newFile.isEmpty()) {
            return;
        }

        mLineEdit->setText(newFile);
        Q_EMIT okEnabled(true);
    });

    layout->addRow(i18nc("@label", "Data Files:"), hlay);

    mFileWatchingCheckBox->setText(i18nc("@option:check", "File watching enabled"));
    connect(mFileWatchingCheckBox, &QCheckBox::toggled, this, [this] {
        Q_EMIT okEnabled(true);
    });
    layout->addRow({}, mFileWatchingCheckBox);
}

void AccountWidget::loadSettings()
{
    QString oldFile = mSettings.dataFile();
    if (oldFile.isEmpty()) {
        oldFile = QDir::homePath();
    }
    mLineEdit->setText(oldFile);
    mFileWatchingCheckBox->setChecked(mSettings.fileWatchingEnabled());
}

void AccountWidget::saveSettings()
{
    mSettings.setDataFile(mLineEdit->text());
    mSettings.setFileWatchingEnabled(mFileWatchingCheckBox->isChecked());
    mSettings.save();
}

#include "moc_accountwidget.cpp"
