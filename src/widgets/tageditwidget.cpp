/*
    This file is part of Akonadi

    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "tageditwidget.h"

#include <KLocalizedString>
#include <QLineEdit>
#include <KMessageBox>
#include <KCheckableProxyModel>
#include <QListView>
#include "changerecorder.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tagfetchscope.h"
#include "tagattribute.h"
#include "tagmodel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace Akonadi;

class Q_DECL_HIDDEN TagEditWidget::Private : public QObject
{
    Q_OBJECT
public:
    Private(Akonadi::TagModel *model, QWidget *parent);

public Q_SLOTS:
    void slotTextEdited(const QString &text);
    void slotItemEntered(const QModelIndex &index);
    void showDeleteButton();
    void deleteTag();
    void slotCreateTag();
    void slotCreateTagFinished(KJob *job);
    void onRowsInserted(const QModelIndex &parent, int start, int end);

public:
    void select(const QModelIndex &parent, int start, int end, QItemSelectionModel::SelectionFlag selectionFlag);
    enum ItemType {
        UrlTag = Qt::UserRole + 1
    };

    QWidget *d;
    Akonadi::Tag::List m_tags;
    Akonadi::TagModel *m_model = nullptr;
    QListView *m_tagsView = nullptr;
    KCheckableProxyModel *m_checkableProxy = nullptr;
    QModelIndex m_deleteCandidate;
    QPushButton *m_newTagButton = nullptr;
    QLineEdit *m_newTagEdit = nullptr;

    QPushButton *m_deleteButton = nullptr;
    QTimer *m_deleteButtonTimer = nullptr;
};

TagEditWidget::Private::Private(Akonadi::TagModel *model, QWidget *parent)
    : QObject()
    , d(parent)
    , m_model(model)
{

}

void TagEditWidget::Private::select(const QModelIndex &parent, int start, int end, QItemSelectionModel::SelectionFlag selectionFlag)
{
    QItemSelection selection;
    for (int i = start; i <= end; i++) {
        const QModelIndex index = m_model->index(i, 0, parent);
        const Akonadi::Tag insertedTag = index.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
        if (m_tags.contains(insertedTag)) {
            selection.select(index, index);
        }
    }
    m_checkableProxy->selectionModel()->select(selection, selectionFlag);
}

void TagEditWidget::Private::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    select(parent, start, end, QItemSelectionModel::Select);
}

void TagEditWidget::Private::slotCreateTag()
{
    if (m_newTagButton->isEnabled()) {
        Akonadi::TagCreateJob *createJob = new Akonadi::TagCreateJob(Akonadi::Tag(m_newTagEdit->text()), this);
        connect(createJob, &Akonadi::TagCreateJob::finished, this, &TagEditWidget::Private::slotCreateTagFinished);

        m_newTagEdit->clear();
        m_newTagEdit->setEnabled(false);
        m_newTagButton->setEnabled(false);
    }
}

void TagEditWidget::Private::slotCreateTagFinished(KJob *job)
{
    if (job->error()) {
        QMessageBox::critical(d, i18n("Failed to create a new tag"),
                              i18n("An error occurred while creating a new tag"));
    }

    m_newTagEdit->setEnabled(true);
}

void TagEditWidget::Private::slotTextEdited(const QString &text)
{
    // Remove unnecessary spaces from a new tag is
    // mandatory, as the user cannot see the difference
    // between a tag "Test" and "Test ".
    const QString tagText = text.simplified();
    if (tagText.isEmpty()) {
        m_newTagButton->setEnabled(false);
        return;
    }

    // Check whether the new tag already exists
    const int count = m_model->rowCount();
    bool exists = false;
    for (int i = 0; i < count; ++i) {
        const QModelIndex index = m_model->index(i, 0, QModelIndex());
        if (index.data(Qt::DisplayRole).toString() == tagText) {
            exists = true;
            break;
        }
    }
    m_newTagButton->setEnabled(!exists);
}

void TagEditWidget::Private::slotItemEntered(const QModelIndex &index)
{
    // align the delete-button to stay on the right border
    // of the item
    const QRect rect = m_tagsView->visualRect(index);
    const int size = rect.height();
    const int x = rect.right() - size;
    const int y = rect.top();
    m_deleteButton->move(x, y);
    m_deleteButton->resize(size, size);

    m_deleteCandidate = index;
    m_deleteButtonTimer->start();
}

void TagEditWidget::Private::showDeleteButton()
{
    m_deleteButton->show();
}

void TagEditWidget::Private::deleteTag()
{
    Q_ASSERT(m_deleteCandidate.isValid());
    const Akonadi::Tag tag = m_deleteCandidate.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
    const QString text = xi18nc("@info",
                                "Do you really want to remove the tag <resource>%1</resource>?",
                                tag.name());
    const QString caption = i18nc("@title", "Delete tag");
    if (KMessageBox::questionYesNo(d, text, caption, KStandardGuiItem::del(), KStandardGuiItem::cancel()) == KMessageBox::Yes) {
        new Akonadi::TagDeleteJob(tag, this);
    }
}

TagEditWidget::TagEditWidget(Akonadi::TagModel *model, QWidget *parent, bool enableSelection)
    : QWidget(parent)
    , d(new Private(model, this))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(d->m_model, this);
    d->m_checkableProxy = new KCheckableProxyModel(this);
    d->m_checkableProxy->setSourceModel(d->m_model);
    d->m_checkableProxy->setSelectionModel(selectionModel);
    connect(d->m_model, &QAbstractItemModel::rowsInserted, d.data(), &Private::onRowsInserted);

    d->m_tagsView = new QListView(this);
    d->m_tagsView->setMouseTracking(true);
    d->m_tagsView->setSelectionMode(QAbstractItemView::NoSelection);
    d->m_tagsView->installEventFilter(this);
    if (enableSelection) {
        d->m_tagsView->setModel(d->m_checkableProxy);
    } else {
        d->m_tagsView->setModel(d->m_model);
    }
    connect(d->m_tagsView, &QAbstractItemView::entered,
            d.data(), &Private::slotItemEntered);

    d->m_newTagEdit = new QLineEdit(this);
    d->m_newTagEdit->setClearButtonEnabled(true);
    connect(d->m_newTagEdit, &QLineEdit::textEdited,
            d.data(), &Private::slotTextEdited);
    connect(d->m_newTagEdit, &QLineEdit::returnPressed,
            d.data(), &Private::slotCreateTag);

    d->m_newTagButton = new QPushButton(i18nc("@label", "Create new tag"));
    d->m_newTagButton->setEnabled(false);
    connect(d->m_newTagButton, &QAbstractButton::clicked,
            d.data(), &Private::slotCreateTag);

    QHBoxLayout *newTagLayout = new QHBoxLayout();
    newTagLayout->addWidget(d->m_newTagEdit, 1);
    newTagLayout->addWidget(d->m_newTagButton);

    if (enableSelection) {
        QLabel *label = new QLabel(i18nc("@label:textbox",
                                         "Configure which tags should "
                                         "be applied."), this);
        topLayout->addWidget(label);
    }
    topLayout->addWidget(d->m_tagsView);
    topLayout->addLayout(newTagLayout);

    // create the delete button, which is shown when
    // hovering the items
    d->m_deleteButton = new QPushButton(d->m_tagsView->viewport());
    d->m_deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    d->m_deleteButton->setToolTip(i18nc("@info", "Delete tag"));
    d->m_deleteButton->hide();
    connect(d->m_deleteButton, &QAbstractButton::clicked, d.data(), &Private::deleteTag);

    d->m_deleteButtonTimer = new QTimer(this);
    d->m_deleteButtonTimer->setSingleShot(true);
    d->m_deleteButtonTimer->setInterval(500);
    connect(d->m_deleteButtonTimer, &QTimer::timeout, d.data(), &Private::showDeleteButton);
}

TagEditWidget::~TagEditWidget()
{

}

void TagEditWidget::setSelection(const Akonadi::Tag::List &tags)
{
    d->m_tags = tags;
    d->select(QModelIndex(), 0, d->m_model->rowCount() - 1, QItemSelectionModel::ClearAndSelect);
}

Akonadi::Tag::List TagEditWidget::selection() const
{
    Akonadi::Tag::List list;
    for (int i = 0; i < d->m_checkableProxy->rowCount(); ++i) {
        if (d->m_checkableProxy->selectionModel()->isRowSelected(i, QModelIndex())) {
            const QModelIndex index = d->m_checkableProxy->index(i, 0, QModelIndex());
            const Akonadi::Tag tag = index.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
            list << tag;
        }
    }
    return list;
}

bool TagEditWidget::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == d->m_tagsView) && (event->type() == QEvent::Leave)) {
        d->m_deleteButtonTimer->stop();
        d->m_deleteButton->hide();
    }
    return QWidget::eventFilter(watched, event);
}

#include "tageditwidget.moc"
