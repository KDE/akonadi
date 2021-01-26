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

class Q_DECL_HIDDEN CachePolicyPage::Private
{
public:
    Private()
        : mUi(new Ui::CachePolicyPage)
    {
    }

    ~Private()
    {
        delete mUi;
    }

    void slotIntervalValueChanged(int /*interval*/);
    void slotCacheValueChanged(int /*interval*/);
    void slotRetrievalOptionsGroupBoxDisabled(bool disable);

    Ui::CachePolicyPage *const mUi;
};

void CachePolicyPage::Private::slotIntervalValueChanged(int interval)
{
    mUi->checkInterval->setSuffix(QLatin1Char(' ') + i18np("minute", "minutes", interval));
}

void CachePolicyPage::Private::slotCacheValueChanged(int interval)
{
    mUi->localCacheTimeout->setSuffix(QLatin1Char(' ') + i18np("minute", "minutes", interval));
}

void CachePolicyPage::Private::slotRetrievalOptionsGroupBoxDisabled(bool disable)
{
    mUi->retrievalOptionsGroupBox->setDisabled(disable);
    if (!disable) {
        mUi->label->setEnabled(mUi->retrieveOnlyHeaders->isChecked());
        mUi->localCacheTimeout->setEnabled(mUi->retrieveOnlyHeaders->isChecked());
    }
}

CachePolicyPage::CachePolicyPage(QWidget *parent, GuiMode mode)
    : CollectionPropertiesPage(parent)
    , d(new Private)
{
    setObjectName(QStringLiteral("Akonadi::CachePolicyPage"));
    setPageTitle(i18n("Retrieval"));

    d->mUi->setupUi(this);
    connect(d->mUi->checkInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        d->slotIntervalValueChanged(value);
    });
    connect(d->mUi->localCacheTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        d->slotCacheValueChanged(value);
    });
    connect(d->mUi->inherit, &QCheckBox::toggled, this, [this](bool checked) {
        d->slotRetrievalOptionsGroupBoxDisabled(checked);
    });
    if (mode == AdvancedMode) {
        d->mUi->stackedWidget->setCurrentWidget(d->mUi->rawPage);
    }
}

CachePolicyPage::~CachePolicyPage()
{
    delete d;
}

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

    d->mUi->inherit->setChecked(policy.inheritFromParent());
    d->mUi->checkInterval->setValue(interval);
    d->mUi->localCacheTimeout->setValue(cache);
    d->mUi->syncOnDemand->setChecked(policy.syncOnDemand());
    d->mUi->localParts->setItems(policy.localParts());

    const bool fetchBodies = policy.localParts().contains(QLatin1String("RFC822"));
    d->mUi->retrieveFullMessages->setChecked(fetchBodies);

    // done explicitly to disable/enabled widgets
    d->mUi->retrieveOnlyHeaders->setChecked(!fetchBodies);
    d->mUi->label->setEnabled(!fetchBodies);
    d->mUi->localCacheTimeout->setEnabled(!fetchBodies);
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
    if (d->mUi->stackedWidget->currentWidget() != d->mUi->rawPage) {
        if (d->mUi->retrieveFullMessages->isChecked() && !localParts.contains(QLatin1String("RFC822"))) {
            localParts.append(QStringLiteral("RFC822"));
        } else if (!d->mUi->retrieveFullMessages->isChecked() && localParts.contains(QLatin1String("RFC822"))) {
            localParts.removeAll(QStringLiteral("RFC822"));
        }
    }

    policy.setLocalParts(localParts);
    collection.setCachePolicy(policy);
}

#include "moc_cachepolicypage.cpp"
