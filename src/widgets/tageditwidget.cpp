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
#include "ui_tageditwidget.h"
#include "changerecorder.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tagfetchscope.h"
#include "tagattribute.h"
#include "tagmodel.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KCheckableProxyModel>

#include <QEvent>
#include <QPushButton>

using namespace Akonadi;

class Q_DECL_HIDDEN TagEditWidget::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(QWidget *parent);

public Q_SLOTS:
    void slotTextEdited(const QString &text);
    void slotItemEntered(const QModelIndex &index);
    void deleteTag();
    void slotCreateTag();
    void slotCreateTagFinished(KJob *job);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onModelPopulated();

public:
    void initCheckableProxy(Akonadi::TagModel *model)
    {
        Q_ASSERT(m_checkableProxy);

        QItemSelectionModel *selectionModel = new QItemSelectionModel(model, m_checkableProxy.get());
        m_checkableProxy->setSourceModel(model);
        m_checkableProxy->setSelectionModel(selectionModel);
    }

    void select(const QModelIndex &parent, int start, int end, QItemSelectionModel::SelectionFlag selectionFlag) const;
    enum ItemType {
        UrlTag = Qt::UserRole + 1
    };

    QWidget * const d;
    Ui::TagEditWidget ui;

    Akonadi::Tag::List m_tags;
    Akonadi::TagModel *m_model = nullptr;
    QScopedPointer<KCheckableProxyModel> m_checkableProxy;
    QModelIndex m_deleteCandidate;

    QPushButton *m_deleteButton = nullptr;
};

TagEditWidget::Private::Private(QWidget *parent)
    : d(parent)
{}

void TagEditWidget::Private::select(const QModelIndex &parent, int start, int end, QItemSelectionModel::SelectionFlag selectionFlag) const
{
    if (!m_model) {
        return;
    }

    QItemSelection selection;
    for (int i = start; i <= end; i++) {
        const QModelIndex index = m_model->index(i, 0, parent);
        const Akonadi::Tag insertedTag = index.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
        if (m_tags.contains(insertedTag)) {
            selection.select(index, index);
        }
    }
    if (m_checkableProxy) {
        m_checkableProxy->selectionModel()->select(selection, selectionFlag);
    }
}

void TagEditWidget::Private::onModelPopulated()
{
    select(QModelIndex(), 0, m_model->rowCount() - 1, QItemSelectionModel::ClearAndSelect);
}

void TagEditWidget::Private::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    select(parent, start, end, QItemSelectionModel::Select);
}

void TagEditWidget::Private::slotCreateTag()
{
    if (ui.newTagButton->isEnabled()) {
        auto *createJob = new TagCreateJob(Akonadi::Tag(ui.newTagEdit->text()), this);
        connect(createJob, &TagCreateJob::finished, this, &TagEditWidget::Private::slotCreateTagFinished);

        ui.newTagEdit->clear();
        ui.newTagEdit->setEnabled(false);
        ui.newTagButton->setEnabled(false);
    }
}

void TagEditWidget::Private::slotCreateTagFinished(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(d, i18n("Failed to create a new tag"),
                              i18n("An error occurred while creating a new tag"));
    }

    ui.newTagEdit->setEnabled(true);
}

void TagEditWidget::Private::slotTextEdited(const QString &text)
{
    // Remove unnecessary spaces from a new tag is
    // mandatory, as the user cannot see the difference
    // between a tag "Test" and "Test ".
    const QString tagText = text.simplified();
    if (tagText.isEmpty()) {
        ui.newTagButton->setEnabled(false);
        return;
    }

    // Check whether the new tag already exists
    bool exists = false;
    for (int i = 0, count = m_model->rowCount(); i < count; ++i) {
        const QModelIndex index = m_model->index(i, 0, QModelIndex());
        if (index.data(Qt::DisplayRole).toString() == tagText) {
            exists = true;
            break;
        }
    }
    ui.newTagButton->setEnabled(!exists);
}

void TagEditWidget::Private::slotItemEntered(const QModelIndex &index)
{
    // align the delete-button to stay on the right border
    // of the item
    const QRect rect = ui.tagsView->visualRect(index);
    const int size = rect.height();
    const int x = rect.right() - size;
    const int y = rect.top();
    m_deleteButton->move(x, y);
    m_deleteButton->resize(size, size);

    m_deleteCandidate = index;
    m_deleteButton->show();
}

void TagEditWidget::Private::deleteTag()
{
    Q_ASSERT(m_deleteCandidate.isValid());
    const auto tag = m_deleteCandidate.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
    const QString text = xi18nc("@info",
                                "Do you really want to remove the tag <resource>%1</resource>?",
                                tag.name());
    const QString caption = i18nc("@title", "Delete tag");
    if (KMessageBox::questionYesNo(d, text, caption, KStandardGuiItem::del(), KStandardGuiItem::cancel()) == KMessageBox::Yes) {
        new TagDeleteJob(tag, this);
    }
}

TagEditWidget::TagEditWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->ui.setupUi(this);


    d->ui.tagsView->installEventFilter(this);
    connect(d->ui.tagsView, &QAbstractItemView::entered, d.get(), &Private::slotItemEntered);

    connect(d->ui.newTagEdit, &QLineEdit::textEdited, d.get(), &Private::slotTextEdited);
    connect(d->ui.newTagEdit, &QLineEdit::returnPressed, d.get(), &Private::slotCreateTag);
    connect(d->ui.newTagButton, &QAbstractButton::clicked, d.get(), &Private::slotCreateTag);


    // create the delete button, which is shown when
    // hovering the items
    d->m_deleteButton = new QPushButton(d->ui.tagsView->viewport());
    d->m_deleteButton->setObjectName(QStringLiteral("tagDeleteButton"));
    d->m_deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    d->m_deleteButton->setToolTip(i18nc("@info", "Delete tag"));
    d->m_deleteButton->hide();
    connect(d->m_deleteButton, &QAbstractButton::clicked, d.data(), &Private::deleteTag);
}

TagEditWidget::TagEditWidget(Akonadi::TagModel *model, QWidget *parent, bool enableSelection)
    : TagEditWidget(parent)
{
    setModel(model);
    setSelectionEnabled(enableSelection);
}

TagEditWidget::~TagEditWidget() = default;

void TagEditWidget::setSelectionEnabled(bool enabled)
{
    if (enabled == (d->m_checkableProxy != nullptr)) {
        return;
    }

    if (enabled) {
        d->m_checkableProxy.reset(new KCheckableProxyModel(this));
        if (d->m_model) {
            d->initCheckableProxy(d->m_model);
        }
        d->ui.tagsView->setModel(d->m_checkableProxy.get());
    } else {
        d->m_checkableProxy.reset();
        d->ui.tagsView->setModel(d->m_model);
    }
    d->ui.selectLabel->setVisible(enabled);
}

void TagEditWidget::setModel(TagModel *model)
{
    if (d->m_model) {
        disconnect(d->m_model, &QAbstractItemModel::rowsInserted, d.get(), &Private::onRowsInserted);
        disconnect(d->m_model, &TagModel::populated, d.get(), &Private::onModelPopulated);
    }

    d->m_model = model;
    if (d->m_model) {
        connect(d->m_model, &QAbstractItemModel::rowsInserted, d.get(), &Private::onRowsInserted);
        if (d->m_checkableProxy) {
            d->initCheckableProxy(d->m_model);
            d->ui.tagsView->setModel(d->m_checkableProxy.get());
        } else {
            d->ui.tagsView->setModel(d->m_model);
        }
        connect(d->m_model, &TagModel::populated, d.get(), &Private::onModelPopulated);
    }
}

TagModel *TagEditWidget::model() const
{
    return d->m_model;
}

bool TagEditWidget::selectionEnabled() const
{
    return d->m_checkableProxy != nullptr;
}

void TagEditWidget::setSelection(const Akonadi::Tag::List &tags)
{
    d->m_tags = tags;
    d->select(QModelIndex(), 0, d->m_model->rowCount() - 1, QItemSelectionModel::ClearAndSelect);
}

Akonadi::Tag::List TagEditWidget::selection() const
{
    if (!d->m_checkableProxy) {
        return {};
    }

    Akonadi::Tag::List list;
    for (int i = 0; i < d->m_checkableProxy->rowCount(); ++i) {
        if (d->m_checkableProxy->selectionModel()->isRowSelected(i, QModelIndex())) {
            const auto index = d->m_checkableProxy->index(i, 0, QModelIndex());
            const auto tag = index.data(TagModel::TagRole).value<Tag>();
            list.push_back(tag);
        }
    }
    return list;
}

bool TagEditWidget::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == d->ui.tagsView) && (event->type() == QEvent::Leave)) {
        d->m_deleteButton->hide();
    }
    return QWidget::eventFilter(watched, event);
}

#include "tageditwidget.moc"

