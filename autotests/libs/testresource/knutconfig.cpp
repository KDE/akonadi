// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <Akonadi/AgentConfigurationBase>

#include "accountwidget.h"
#include "settings.h"

class KnutConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    KnutConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mSettings(config)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &AccountWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget.loadSettings();
    }

    [[nodiscard]] bool save() const override
    {
        const_cast<KnutConfig *>(this)->mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    KnutSettings mSettings;
    AccountWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(KnutConfigFactory, "knutconfig.json", KnutConfig)

#include "knutconfig.moc"
