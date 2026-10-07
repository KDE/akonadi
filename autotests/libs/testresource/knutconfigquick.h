// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#pragma once

#include <Akonadi/AgentConfigurationQuickBase>

#include "settings.h"

#include <QUrl>
#include <qqmlregistration.h>

class KnutConfigQuick : public Akonadi::AgentConfigurationQuickBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the Knut configuration plugin")
    Q_PROPERTY(QString dataFile READ dataFile WRITE setDataFile NOTIFY dataFileChanged)
    Q_PROPERTY(bool fileWatchingEnabled READ fileWatchingEnabled WRITE setFileWatchingEnabled NOTIFY fileWatchingEnabledChanged)
public:
    KnutConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent);

    [[nodiscard]] QString dataFile() const;
    void setDataFile(const QString &dataFile);
    [[nodiscard]] bool fileWatchingEnabled() const;
    void setFileWatchingEnabled(bool enabled);
    Q_INVOKABLE void setDataFileUrl(const QUrl &url);

    void load() override;
    bool save() override;

Q_SIGNALS:
    void dataFileChanged();
    void fileWatchingEnabledChanged();

private:
    KnutSettings m_settings;
    QString m_dataFile;
    bool m_fileWatchingEnabled = true;
};
