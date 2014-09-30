/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "collectionrequester.h"
#include "collectiondialog.h"
#include "entitydisplayattribute.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"

#include <klineedit.h>
#include <klocalizedstring.h>
#include <kstandardshortcut.h>

#include <QtCore/QEvent>
#include <QAction>
#include <QPushButton>
#include <QHBoxLayout>

using namespace Akonadi;

class CollectionRequester::Private
{
public:
    Private(CollectionRequester *parent)
        : q(parent)
        , edit(0)
        , button(0)
        , collectionDialog(0)
    {
    }

    ~Private()
    {
    }

    void fetchCollection(const Collection &collection);

    void init();

    // slots
    void _k_slotOpenDialog();
    void _k_collectionReceived(KJob *job);
    void _k_collectionsNamesReceived(KJob *job);

    CollectionRequester *q;
    Collection collection;
    QLineEdit *edit;
    QPushButton *button;
    CollectionDialog *collectionDialog;
};

void CollectionRequester::Private::fetchCollection(const Collection &collection)
{
    CollectionFetchJob *job = new CollectionFetchJob(collection, Akonadi::CollectionFetchJob::Base, q);
    job->setProperty("OriginalCollectionId", collection.id());
    job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
    connect(job, SIGNAL(finished(KJob*)), q, SLOT(_k_collectionReceived(KJob*)));
}

void CollectionRequester::Private::_k_collectionReceived(KJob *job)
{
    CollectionFetchJob *fetch = qobject_cast<CollectionFetchJob *>(job);
    Collection::List chain;
    if (fetch->collections().size() == 1) {
        Collection currentCollection = fetch->collections().first();
        while (currentCollection.isValid()) {
            chain << currentCollection;
            currentCollection = Collection(currentCollection.parentCollection());
        }

        CollectionFetchJob *namesFetch = new CollectionFetchJob(chain, CollectionFetchJob::Base, q);
        namesFetch->setProperty("OriginalCollectionId", job->property("OriginalCollectionId"));
        namesFetch->fetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
        connect(namesFetch, SIGNAL(finished(KJob*)), q, SLOT(_k_collectionsNamesReceived(KJob*)));
    } else {
        _k_collectionsNamesReceived(job);
    }
}

void CollectionRequester::Private::_k_collectionsNamesReceived(KJob *job)
{
    CollectionFetchJob *fetch = qobject_cast<CollectionFetchJob *>(job);
    const qint64 originalId = fetch->property("OriginalCollectionId").toLongLong();

    QMap<qint64, Collection> names;
    Q_FOREACH (const Collection &collection, fetch->collections()) {
        names.insert(collection.id(), collection);
    }

    QStringList namesList;
    Collection currentCollection = names.take(originalId);
    while (currentCollection.isValid()) {
        namesList.prepend(currentCollection.displayName());
        currentCollection = names.take(currentCollection.parent());
    }
    edit->setText(namesList.join(QLatin1String("/")));
}

void CollectionRequester::Private::init()
{
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(0);
    q->setLayout(hbox);

    edit = new QLineEdit(q);
    edit->setReadOnly(true);
    edit->setPlaceholderText(i18n("No Folder"));
    edit->setClearButtonEnabled(false);
    edit->setFocusPolicy(Qt::NoFocus);
    hbox->addWidget(edit);

    button = new QPushButton(q);
    button->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    const int buttonSize = edit->sizeHint().height();
    button->setFixedSize(buttonSize, buttonSize);
    button->setToolTip(i18n("Open collection dialog"));
    hbox->addWidget(button);

    hbox->setSpacing(-1);

    edit->installEventFilter(q);
    q->setFocusProxy(button);
    q->setFocusPolicy(Qt::StrongFocus);

    q->connect(button, SIGNAL(clicked()), q, SLOT(_k_slotOpenDialog()));

    QAction *openAction = new QAction(q);
    openAction->setShortcut(KStandardShortcut::Open);
    q->connect(openAction, SIGNAL(triggered(bool)), q, SLOT(_k_slotOpenDialog()));

    collectionDialog = new CollectionDialog(q);
    collectionDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi")));
    collectionDialog->setCaption(i18n("Select a collection"));
    collectionDialog->setSelectionMode(QAbstractItemView::SingleSelection);
    collectionDialog->changeCollectionDialogOptions(CollectionDialog::KeepTreeExpanded);
}

void CollectionRequester::Private::_k_slotOpenDialog()
{
    CollectionDialog *dlg = collectionDialog;

    if (dlg->exec() != QDialog::Accepted) {
        return;
    }

    const Akonadi::Collection collection = dlg->selectedCollection();
    q->setCollection(collection);
    emit q->collectionChanged(collection);
}

CollectionRequester::CollectionRequester(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->init();
}

CollectionRequester::CollectionRequester(const Akonadi::Collection &collection, QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->init();
    setCollection(collection);
}

CollectionRequester::~CollectionRequester()
{
    delete d;
}

Collection CollectionRequester::collection() const
{
    return d->collection;
}

void CollectionRequester::setCollection(const Collection &collection)
{
    d->collection = collection;
    QString name;
    if (collection.isValid()) {
        name = collection.displayName();
    }

    d->edit->setText(name);
    emit collectionChanged(collection);
    d->fetchCollection(collection);
}

void CollectionRequester::setMimeTypeFilter(const QStringList &mimeTypes)
{
    if (d->collectionDialog) {
        d->collectionDialog->setMimeTypeFilter(mimeTypes);
    }
}

QStringList CollectionRequester::mimeTypeFilter() const
{
    if (d->collectionDialog) {
        return d->collectionDialog->mimeTypeFilter();
    } else {
        return QStringList();
    }
}

void CollectionRequester::setAccessRightsFilter(Collection::Rights rights)
{
    if (d->collectionDialog) {
        d->collectionDialog->setAccessRightsFilter(rights);
    }
}

Collection::Rights CollectionRequester::accessRightsFilter() const
{
    if (d->collectionDialog) {
        return d->collectionDialog->accessRightsFilter();
    } else {
        return Akonadi::Collection::ReadOnly;
    }
}

void CollectionRequester::changeCollectionDialogOptions(CollectionDialog::CollectionDialogOptions options)
{
    if (d->collectionDialog) {
        d->collectionDialog->changeCollectionDialogOptions(options);
    }
}

void CollectionRequester::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowTitleChange) {
        if (d->collectionDialog) {
            d->collectionDialog->setCaption(windowTitle());
        }
    } else if (event->type() == QEvent::EnabledChange) {
        if (d->collectionDialog) {
            d->collectionDialog->setEnabled(true);
        }
    }
    QWidget::changeEvent(event);
}

#include "moc_collectionrequester.cpp"
