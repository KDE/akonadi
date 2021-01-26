/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionpropertiesdialog.h"

#include "cachepolicy.h"
#include "cachepolicypage.h"
#include "collection.h"
#include "collectiongeneralpropertiespage_p.h"
#include "collectionmodifyjob.h"

#include "akonadiwidgets_debug.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QTabWidget>

#include <QDialogButtonBox>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN CollectionPropertiesDialog::Private
{
public:
    Private(CollectionPropertiesDialog *parent, const Akonadi::Collection &collection, const QStringList &pageNames);

    void init();

    static void registerBuiltinPages();

    void save()
    {
        const int numberOfTab(mTabWidget->count());
        for (int i = 0; i < numberOfTab; ++i) {
            auto *page = static_cast<CollectionPropertiesPage *>(mTabWidget->widget(i));
            page->save(mCollection);
        }

        // We use WA_DeleteOnClose => Don't use dialog as parent otherwise we can't save modified collection.
        auto *job = new CollectionModifyJob(mCollection);
        connect(job, &CollectionModifyJob::result, q, [this](KJob *job) {
            saveResult(job);
        });
        Q_EMIT q->settingsSaved();
    }

    void saveResult(KJob *job)
    {
        if (job->error()) {
            // TODO
            qCWarning(AKONADIWIDGETS_LOG) << job->errorString();
        }
    }

    void setCurrentPage(const QString &name)
    {
        const int numberOfTab(mTabWidget->count());
        for (int i = 0; i < numberOfTab; ++i) {
            QWidget *w = mTabWidget->widget(i);
            if (w->objectName() == name) {
                mTabWidget->setCurrentIndex(i);
                break;
            }
        }
    }

    CollectionPropertiesDialog *const q;
    Collection mCollection;
    QStringList mPageNames;
    QTabWidget *mTabWidget = nullptr;
};

class CollectionPropertiesPageFactoryList : public QList<CollectionPropertiesPageFactory *>
{
public:
    explicit CollectionPropertiesPageFactoryList() = default;
    CollectionPropertiesPageFactoryList(const CollectionPropertiesPageFactoryList &) = delete;
    CollectionPropertiesPageFactoryList &operator=(const CollectionPropertiesPageFactoryList &) = delete;
    ~CollectionPropertiesPageFactoryList()
    {
        qDeleteAll(*this);
    }
};

Q_GLOBAL_STATIC(CollectionPropertiesPageFactoryList, s_pages) // NOLINT(readability-redundant-member-init)

static bool s_defaultPage = true;

CollectionPropertiesDialog::Private::Private(CollectionPropertiesDialog *qq, const Akonadi::Collection &collection, const QStringList &pageNames)
    : q(qq)
    , mCollection(collection)
    , mPageNames(pageNames)
    , mTabWidget(nullptr)
{
    if (s_defaultPage) {
        registerBuiltinPages();
    }
}

void CollectionPropertiesDialog::Private::registerBuiltinPages()
{
    static bool registered = false;

    if (registered) {
        return;
    }

    s_pages->append(new CollectionGeneralPropertiesPageFactory());
    s_pages->append(new CachePolicyPageFactory());

    registered = true;
}

void CollectionPropertiesDialog::Private::init()
{
    auto *mainLayout = new QVBoxLayout(q);
    q->setAttribute(Qt::WA_DeleteOnClose);
    mTabWidget = new QTabWidget(q);
    mainLayout->addWidget(mTabWidget);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return); // NOLINT(bugprone-suspicious-enum-usage)
    q->connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
    q->connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (mPageNames.isEmpty()) { // default loading
        for (CollectionPropertiesPageFactory *factory : qAsConst(*s_pages)) {
            CollectionPropertiesPage *page = factory->createWidget(mTabWidget);
            if (page->canHandle(mCollection)) {
                mTabWidget->addTab(page, page->pageTitle());
                page->load(mCollection);
            } else {
                delete page;
            }
        }
    } else { // custom loading
        QHash<QString, CollectionPropertiesPage *> pages;

        for (CollectionPropertiesPageFactory *factory : qAsConst(*s_pages)) {
            CollectionPropertiesPage *page = factory->createWidget(mTabWidget);
            const QString pageName = page->objectName();

            if (page->canHandle(mCollection) && mPageNames.contains(pageName) && !pages.contains(pageName)) {
                pages.insert(page->objectName(), page);
            } else {
                delete page;
            }
        }

        for (const QString &pageName : qAsConst(mPageNames)) {
            CollectionPropertiesPage *page = pages.value(pageName);
            if (page) {
                mTabWidget->addTab(page, page->pageTitle());
                page->load(mCollection);
            }
        }
    }

    q->connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, q, [this]() {
        save();
    });
    q->connect(buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, q, &QObject::deleteLater);

    KConfigGroup group(KSharedConfig::openConfig(), "CollectionPropertiesDialog");
    const QSize size = group.readEntry("Size", QSize());
    if (size.isValid()) {
        q->resize(size);
    } else {
        q->resize(q->sizeHint().width(), q->sizeHint().height());
    }
}

CollectionPropertiesDialog::CollectionPropertiesDialog(const Collection &collection, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this, collection, QStringList()))
{
    d->init();
}

CollectionPropertiesDialog::CollectionPropertiesDialog(const Collection &collection, const QStringList &pages, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this, collection, pages))
{
    d->init();
}

CollectionPropertiesDialog::~CollectionPropertiesDialog()
{
    KConfigGroup group(KSharedConfig::openConfig(), "CollectionPropertiesDialog");
    group.writeEntry("Size", size());
    delete d;
}

void CollectionPropertiesDialog::registerPage(CollectionPropertiesPageFactory *factory)
{
    if (s_pages->isEmpty() && s_defaultPage) {
        Private::registerBuiltinPages();
    }
    s_pages->append(factory);
}

void CollectionPropertiesDialog::useDefaultPage(bool defaultPage)
{
    s_defaultPage = defaultPage;
}

QString CollectionPropertiesDialog::defaultPageObjectName(DefaultPage page)
{
    switch (page) {
    case GeneralPage:
        return QStringLiteral("Akonadi::CollectionGeneralPropertiesPage");
    case CachePage:
        return QStringLiteral("Akonadi::CachePolicyPage");
    }

    return QString();
}

void CollectionPropertiesDialog::setCurrentPage(const QString &name)
{
    d->setCurrentPage(name);
}

#include "moc_collectionpropertiesdialog.cpp"
