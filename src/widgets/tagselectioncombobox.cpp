/*
  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
  SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tagselectioncombobox.h"

#include "monitor.h"
#include "tagmodel.h"

#include <KCheckableProxyModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLocale>

#include <KLocalizedString>

#include <algorithm>
#include <variant>

using namespace Akonadi;

namespace
{
template<typename List>
List tagsFromSelection(const QItemSelection &selection, int role)
{
    List tags;
    for (int i = 0; i < selection.size(); ++i) {
        const auto indexes = selection.at(i).indexes();
        std::transform(indexes.cbegin(), indexes.cend(), std::back_inserter(tags), [role](const auto &idx) {
            return idx.model()->data(idx, role).template value<typename List::value_type>();
        });
    }
    return tags;
}

QString getEditText(const QItemSelection &selection)
{
    const auto tags = tagsFromSelection<Tag::List>(selection, TagModel::TagRole);
    QStringList names;
    names.reserve(tags.size());
    std::transform(tags.cbegin(), tags.cend(), std::back_inserter(names), std::bind(&Tag::name, std::placeholders::_1));
    return QLocale{}.createSeparatedList(names);
}

} // namespace

class Akonadi::TagSelectionComboBoxPrivate
{
public:
    explicit TagSelectionComboBoxPrivate(TagSelectionComboBox *parent)
        : q(parent)
    {
    }

    enum LoopControl {
        Break,
        Continue,
    };

    template<typename Selection, typename Comp>
    void setSelection(const Selection &entries, Comp &&cmp)
    {
        if (!mModelReady) {
            mPendingSelection = entries;
            return;
        }

        const auto forEachIndex = [this, entries, cmp](auto &&func) {
            for (int i = 0, cnt = tagModel->rowCount(); i < cnt; ++i) {
                const auto index = tagModel->index(i, 0);
                const auto tag = tagModel->data(index, TagModel::TagRole).value<Tag>();
                if (std::any_of(entries.cbegin(), entries.cend(), std::bind(cmp, tag, std::placeholders::_1))) {
                    if (func(index) == Break) {
                        break;
                    }
                }
            }
        };

        if (mCheckable) {
            QItemSelection selection;
            forEachIndex([&selection](const QModelIndex &index) {
                selection.push_back(QItemSelectionRange{index});
                return Continue;
            });
            selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        } else {
            forEachIndex([this](const QModelIndex &index) {
                q->setCurrentIndex(index.row());
                return Break;
            });
        }
    }

    void toggleItem(const QModelIndex &tagModelIndex) const
    {
        selectionModel->select(tagModelIndex, QItemSelectionModel::Toggle);
    }

    void setItemChecked(const QModelIndex &tagModelIndex, Qt::CheckState state) const
    {
        selectionModel->select(tagModelIndex, state == Qt::Checked ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
    }

    void setCheckable(bool checkable)
    {
        if (checkable) {
            selectionModel = std::make_unique<QItemSelectionModel>(tagModel.get(), q);
            checkableProxy = std::make_unique<KCheckableProxyModel>(q);
            checkableProxy->setSourceModel(tagModel.get());
            checkableProxy->setSelectionModel(selectionModel.get());

            tagModel->setParent(nullptr);
            q->setModel(checkableProxy.get());
            tagModel->setParent(q);

            q->setEditable(true);
            q->lineEdit()->setReadOnly(true);
            q->lineEdit()->setPlaceholderText(i18nc("@label Placeholder text in tag selection combobox", "Select tags..."));
            q->lineEdit()->setAlignment(Qt::AlignLeft);

            q->lineEdit()->installEventFilter(q);
            q->view()->installEventFilter(q);
            q->view()->viewport()->installEventFilter(q);

            q->connect(selectionModel.get(), &QItemSelectionModel::selectionChanged, q, [this]() {
                const auto selection = selectionModel->selection();
                q->setEditText(getEditText(selection));
                Q_EMIT q->selectionChanged(tagsFromSelection<Tag::List>(selection, TagModel::TagRole));
            });
            q->connect(q, &QComboBox::activated, selectionModel.get(), [this](int i) {
                if (q->view()->isVisible()) {
                    const auto index = tagModel->index(i, 0);
                    toggleItem(index);
                }
            });
        } else {
            // QComboBox automatically deletes models that it is a parent of
            // which breaks our stuff
            tagModel->setParent(nullptr);
            q->setModel(tagModel.get());
            tagModel->setParent(q);

            if (q->lineEdit()) {
                q->lineEdit()->removeEventFilter(q);
            }
            if (q->view()) {
                q->view()->removeEventFilter(q);
                q->view()->viewport()->removeEventFilter(q);
            }

            q->setEditable(false);

            selectionModel.reset();
            checkableProxy.reset();
        }
    }

    std::unique_ptr<QItemSelectionModel> selectionModel;
    std::unique_ptr<TagModel> tagModel;
    std::unique_ptr<KCheckableProxyModel> checkableProxy;

    bool mCheckable = false;
    bool mAllowHide = true;
    bool mModelReady = false;

    std::variant<std::monostate, Tag::List, QStringList> mPendingSelection;

private:
    TagSelectionComboBox *const q;
};

TagSelectionComboBox::TagSelectionComboBox(QWidget *parent)
    : QComboBox(parent)
    , d(new TagSelectionComboBoxPrivate(this))
{
    auto monitor = new Monitor(this);
    monitor->setObjectName(QLatin1StringView("TagSelectionComboBoxMonitor"));
    monitor->setTypeMonitored(Monitor::Tags);

    d->tagModel = std::make_unique<TagModel>(monitor, this);
    connect(d->tagModel.get(), &TagModel::populated, this, [this]() {
        d->mModelReady = true;
        if (auto list = std::get_if<Tag::List>(&d->mPendingSelection)) {
            setSelection(*list);
        } else if (auto slist = std::get_if<QStringList>(&d->mPendingSelection)) {
            setSelection(*slist);
        }
        d->mPendingSelection = std::monostate{};
    });

    d->setCheckable(d->mCheckable);
}

TagSelectionComboBox::~TagSelectionComboBox() = default;

void TagSelectionComboBox::setCheckable(bool checkable)
{
    if (d->mCheckable != checkable) {
        d->mCheckable = checkable;
        d->setCheckable(d->mCheckable);
    }
}

bool TagSelectionComboBox::checkable() const
{
    return d->mCheckable;
}

void TagSelectionComboBox::setSelection(const Tag::List &tags)
{
    d->setSelection(tags, [](const Tag &a, const Tag &b) {
        return a.name() == b.name();
    });
}

void TagSelectionComboBox::setSelection(const QStringList &tagNames)
{
    d->setSelection(tagNames, [](const Tag &a, const QString &b) {
        return a.name() == b;
    });
}

Tag::List TagSelectionComboBox::selection() const
{
    if (!d->selectionModel) {
        return {currentData(TagModel::TagRole).value<Tag>()};
    }
    return tagsFromSelection<Tag::List>(d->selectionModel->selection(), TagModel::TagRole);
}

QStringList TagSelectionComboBox::selectionNames() const
{
    if (!d->selectionModel) {
        return {currentText()};
    }
    return tagsFromSelection<QStringList>(d->selectionModel->selection(), TagModel::NameRole);
}

void TagSelectionComboBox::hidePopup()
{
    if (d->mAllowHide) {
        QComboBox::hidePopup();
    }
    d->mAllowHide = true;
}

void TagSelectionComboBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
        showPopup();
        event->accept();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
        hidePopup();
        event->accept();
        break;
    default:
        break;
    }
}

bool TagSelectionComboBox::eventFilter(QObject *receiver, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        switch (static_cast<QKeyEvent *>(event)->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Escape:
            hidePopup();
            return true;
        default:
            break;
        }
        break;
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        d->mAllowHide = false;
        if (receiver == lineEdit()) {
            showPopup();
            return true;
        }
        break;
    default:
        break;
    }
    return QComboBox::eventFilter(receiver, event);
}

#include "moc_tagselectioncombobox.cpp"
