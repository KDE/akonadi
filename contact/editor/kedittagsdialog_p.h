/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KEDIT_TAGS_DIALOG_H
#define KEDIT_TAGS_DIALOG_H

#include <kdialog.h>
#include <akonadi/tag.h>
#include <akonadi/tagmodel.h>

#include <QtGui/QListView>

class TagModel;
class KCheckableProxyModel;
class KLineEdit;
class QPushButton;
class QTimer;

/**
 * @brief Dialog to edit a list of Nepomuk tags.
 *
 * It is possible for the user to add existing tags,
 * create new tags or to remove tags.
 *
 * @see KMetaDataConfigurationDialog
 */
class KEditTagsDialog : public KDialog
{
    Q_OBJECT

public:
    explicit KEditTagsDialog(const Akonadi::Tag::List& tags,
                             Akonadi::TagModel *model,
                             QWidget* parent = 0 );

    virtual ~KEditTagsDialog();

    Akonadi::Tag::List tags() const;

    virtual bool eventFilter(QObject* watched, QEvent* event);

protected Q_SLOTS:
    virtual void slotButtonClicked(int button);

private Q_SLOTS:
    void slotTextEdited(const QString &text);
    void slotItemEntered(const QModelIndex &index);
    void showDeleteButton();
    void deleteTag();
    void slotCreateTag();
    void slotCreateTagFinished(KJob *job);

private:
    void writeConfig();
    void readConfig();
    enum ItemType {
        UrlTag = Qt::UserRole + 1
    };

    Akonadi::Tag::List m_tags;
    Akonadi::TagModel *m_model;
    QListView *m_tagsView;
    KCheckableProxyModel *m_checkableProxy;
    QModelIndex m_deleteCandidate;
    QPushButton *m_newTagButton;
    KLineEdit* m_newTagEdit;

    QPushButton* m_deleteButton;
    QTimer* m_deleteButtonTimer;
};

#endif
