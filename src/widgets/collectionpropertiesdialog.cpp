/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "collectionpropertiesdialog.h"

#include "cachepolicy.h"
#include "cachepolicypage.h"
#include "collection.h"
#include "collectiongeneralpropertiespage_p.h"
#include "collectionmodifyjob.h"

#include <qdebug.h>

#include <ksharedconfig.h>
#include <KConfigGroup>
#include <qtabwidget.h>

#include <QBoxLayout>
#include <QDialogButtonBox>

using namespace Akonadi;

/**
 * @internal
 */
class CollectionPropertiesDialog::Private
{
public:
    Private(CollectionPropertiesDialog *parent, const Akonadi::Collection &collection, const QStringList &pageNames);

    void init();

    static void registerBuiltinPages();

    void save()
    {
        for (int i = 0; i < mTabWidget->count(); ++i) {
            CollectionPropertiesPage *page = static_cast<CollectionPropertiesPage *>(mTabWidget->widget(i));
            page->save(mCollection);
        }

        CollectionModifyJob *job = new CollectionModifyJob(mCollection, q);
        connect(job, SIGNAL(result(KJob*)), q, SLOT(saveResult(KJob*)));
    }

    void saveResult(KJob *job)
    {
        if (job->error()) {
            // TODO
            qWarning() << job->errorString();
        }
        q->deleteLater();
    }

    void setCurrentPage(const QString &name)
    {
        for (int i = 0; i < mTabWidget->count(); ++i) {
            QWidget *w = mTabWidget->widget(i);
            if (w->objectName() == name) {
                mTabWidget->setCurrentIndex(i);
                break;
            }
        }
    }

    CollectionPropertiesDialog *q;
    Collection mCollection;
    QStringList mPageNames;
    QTabWidget *mTabWidget;
};

typedef QList<CollectionPropertiesPageFactory *> CollectionPropertiesPageFactoryList;

Q_GLOBAL_STATIC(CollectionPropertiesPageFactoryList, s_pages)

static bool s_defaultPage = true;

CollectionPropertiesDialog::Private::Private(CollectionPropertiesDialog *qq, const Akonadi::Collection &collection, const QStringList &pageNames)
    : q(qq)
    , mCollection(collection)
    , mPageNames(pageNames)
    , mTabWidget(0)
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
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(0);
    q->setLayout(mainLayout);
    mTabWidget = new QTabWidget(q);
    mainLayout->addWidget(mTabWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    q->connect(buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
    q->connect(buttonBox, SIGNAL(rejected()), q, SLOT(reject()));
    mainLayout->addWidget(buttonBox);


    if (mPageNames.isEmpty()) {   // default loading
        foreach (CollectionPropertiesPageFactory *factory, *s_pages) {
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

        foreach (CollectionPropertiesPageFactory *factory, *s_pages) {
            CollectionPropertiesPage *page = factory->createWidget(mTabWidget);
            const QString pageName = page->objectName();

            if (page->canHandle(mCollection) && mPageNames.contains(pageName) && !pages.contains(pageName)) {
                pages.insert(page->objectName(), page);
            } else {
                delete page;
            }
        }

        foreach (const QString &pageName, mPageNames) {
            CollectionPropertiesPage *page = pages.value(pageName);
            if (page) {
                mTabWidget->addTab(page, page->pageTitle());
                page->load(mCollection);
            }
        }
    }

    q->connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), SLOT(save()));
    q->connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), SLOT(deleteLater()));

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
