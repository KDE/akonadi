/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cachepolicypage.h"

#include "ui_cachepolicypage.h"

#include "cachepolicy.h"
#include "collection.h"
#include "collectionutils.h"

#include <KLocalizedString>

using namespace Akonadi;
using namespace Qt::StringLiterals;

class Akonadi::CachePolicyPagePrivate
{
public:
    CachePolicyPagePrivate()
        : mUi(new Ui::CachePolicyPage)
    {
    }

    ~CachePolicyPagePrivate()
    {
        delete mUi;
    }

    void slotIntervalValueChanged(int /*interval*/);
    void slotCacheValueChanged(int /*interval*/);
    void slotRetrievalOptionsGroupBoxDisabled(bool disable);

    Ui::CachePolicyPage *const mUi;
    CachePolicyPage::GuiMode mode;
};

void CachePolicyPagePrivate::slotIntervalValueChanged(int interval)
{
    mUi->checkInterval->setSuffix(u' ' + i18np("minute", "minutes", interval));
}

void CachePolicyPagePrivate::slotCacheValueChanged(int interval)
{
    mUi->localCacheTimeout->setSuffix(u' ' + i18np("minute", "minutes", interval));
}

void CachePolicyPagePrivate::slotRetrievalOptionsGroupBoxDisabled(bool disable)
{
    mUi->retrieveFullMessages->setDisabled(disable);
    mUi->retrieveFullMessages->setDisabled(disable);
    mUi->retrieveOnlyHeaders->setDisabled(disable);
    mUi->localCacheTimeout->setDisabled(disable);
    mUi->retrievalOptionsLabel->setDisabled(disable);
    mUi->label->setDisabled(disable);
    if (!disable) {
        mUi->label->setEnabled(mUi->retrieveOnlyHeaders->isChecked());
        mUi->localCacheTimeout->setEnabled(mUi->retrieveOnlyHeaders->isChecked());
    }
}

CachePolicyPage::CachePolicyPage(QWidget *parent, GuiMode mode)
    : CollectionPropertiesPage(parent)
    , d(new CachePolicyPagePrivate)
{
    setObjectName(QLatin1StringView("Akonadi::CachePolicyPage"));
    setPageTitle(i18n("Retrieval"));
    d->mode = mode;

    d->mUi->setupUi(this);
    connect(d->mUi->checkInterval, &QSpinBox::valueChanged, this, [this](int value) {
        d->slotIntervalValueChanged(value);
    });
    connect(d->mUi->localCacheTimeout, &QSpinBox::valueChanged, this, [this](int value) {
        d->slotCacheValueChanged(value);
    });
    connect(d->mUi->inherit, &QCheckBox::toggled, this, [this](bool checked) {
        d->slotRetrievalOptionsGroupBoxDisabled(checked);
    });
    if (mode == AdvancedMode) {
        d->mUi->retrievalOptionsLabel->hide();
        d->mUi->retrieveFullMessages->hide();
        d->mUi->retrieveOnlyHeaders->hide();
        d->mUi->localCacheTimeout->hide();
    } else {
        d->mUi->localParts->hide();
        d->mUi->localPartsLabel->hide();
    }
}

CachePolicyPage::~CachePolicyPage() = default;

bool Akonadi::CachePolicyPage::canHandle(const Collection &collection) const
{
    return !collection.isVirtual();
}

void CachePolicyPage::load(const Collection &collection)
{
    const CachePolicy policy = collection.cachePolicy();

    int interval = policy.intervalCheckTime();
    if (interval == -1) {
        interval = 0;
    }

    int cache = policy.cacheTimeout();
    if (cache == -1) {
        cache = 0;
    }

    d->mUi->checkInterval->setValue(interval);
    d->mUi->localCacheTimeout->setValue(cache);
    d->mUi->syncOnDemand->setChecked(policy.syncOnDemand());
    d->mUi->localParts->setItems(policy.localParts());

    const bool fetchBodies = policy.localParts().contains(QLatin1StringView("RFC822"));
    d->mUi->retrieveFullMessages->setChecked(fetchBodies);

    // done explicitly to disable/enabled widgets
    d->mUi->retrieveOnlyHeaders->setChecked(!fetchBodies);
    d->mUi->label->setEnabled(!fetchBodies);
    d->mUi->localCacheTimeout->setEnabled(!fetchBodies);
    // last code otherwise it will call slotRetrievalOptionsGroupBoxDisabled before
    // calling d->mUi->label->setEnabled(!fetchBodies);
    d->mUi->inherit->setChecked(policy.inheritFromParent());

    const auto containsEmails = collection.contentMimeTypes().contains(u"message/rfc822"_s);
    d->mUi->retrievalOptionsLabel->setVisible(containsEmails);
    d->mUi->retrieveFullMessages->setVisible(containsEmails);
    d->mUi->retrieveOnlyHeaders->setVisible(containsEmails);
    d->mUi->label->setVisible(containsEmails);
    d->mUi->localCacheTimeout->setVisible(containsEmails);
}

void CachePolicyPage::save(Collection &collection)
{
    int interval = d->mUi->checkInterval->value();
    if (interval == 0) {
        interval = -1;
    }

    int cache = d->mUi->localCacheTimeout->value();
    if (cache == 0) {
        cache = -1;
    }

    CachePolicy policy = collection.cachePolicy();
    policy.setInheritFromParent(d->mUi->inherit->isChecked());
    policy.setIntervalCheckTime(interval);
    policy.setCacheTimeout(cache);
    policy.setSyncOnDemand(d->mUi->syncOnDemand->isChecked());

    QStringList localParts = d->mUi->localParts->items();

    // Unless we are in "raw" mode, add "bodies" to the list of message
    // parts to keep around locally, if the user selected that, or remove
    // it otherwise. In "raw" mode we simple use the values from the list
    // view.
    if (d->mode != AdvancedMode) {
        if (d->mUi->retrieveFullMessages->isChecked() && !localParts.contains(QLatin1StringView("RFC822"))) {
            localParts.append(QStringLiteral("RFC822"));
        } else if (!d->mUi->retrieveFullMessages->isChecked() && localParts.contains(QLatin1StringView("RFC822"))) {
            localParts.removeAll(QStringLiteral("RFC822"));
        }
    }

    policy.setLocalParts(localParts);
    collection.setCachePolicy(policy);
}

#include "moc_cachepolicypage.cpp"
